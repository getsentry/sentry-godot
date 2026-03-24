using System;
using System.Collections.Concurrent;
using System.Diagnostics.Tracing;
using System.Runtime.ExceptionServices;
using Godot;
using Sentry.Godot.Internal;

namespace Sentry.Godot.ExceptionHandling;

/// <summary>
/// Detects "unhandled" C# exceptions in Godot by combining FirstChanceException
/// with .NET runtime EventSource events. When ExceptionCatchStart reports a Godot
/// bridge method as the catcher, the exception is classified as unhandled by user
/// code and sent to Sentry.
///
/// See: https://learn.microsoft.com/en-us/dotnet/fundamentals/diagnostics/runtime-exception-events
///
/// Key insight: FirstChanceException runs on the throwing thread, but EventListener
/// callbacks run on a dedicated background thread. We correlate via OS thread ID.
/// </summary>
internal class CoreClrExceptionHandler : EventListener {
	private const int ExceptionCatchStart_EventId = 250;
	private const long ExceptionKeyword = 0x8000;

	// Godot bridge classes that wrap user code in try-catch.
	// ExceptionCatchStart passes MethodName containing full IL signature, e.g.:
	//   "valuetype ... [GodotSharp] Godot.Bridge.CSharpInstanceBridge::Call(...)"
	private static readonly string[] _godotBridgeClasses = new[] {
		"Godot.Bridge.CSharpInstanceBridge::",
		"Godot.Bridge.ScriptManagerBridge::",
		"Godot.DelegateUtils::",
		"Godot.SignalAwaiter::",
		"Godot.GD::",
	};

	// FIFO queue of exceptions per OS thread ID.
	// FirstChanceException enqueues via gettid(), ExceptionCatchStart dequeues via eventData.OSThreadId.
	// A queue is needed because multiple FirstChanceException events fire synchronously on the
	// throwing thread before the EventListener (on a separate thread) processes any of them.
	private readonly ConcurrentDictionary<long, ConcurrentQueue<Exception>> _threadExceptions = new();

	// Re-entrancy guard per thread.
	[ThreadStatic]
	private static bool _isProcessing;

	public CoreClrExceptionHandler() {
		AppDomain.CurrentDomain.FirstChanceException += OnFirstChanceException;
	}

	private void OnFirstChanceException(object? sender, FirstChanceExceptionEventArgs e) {
		if (_isProcessing) {
			return;
		}

		try {
			_isProcessing = true;

			long osThreadId = NativeThreadId.GetCurrentThreadId();
			var queue = _threadExceptions.GetOrAdd(osThreadId,
					_ => new ConcurrentQueue<Exception>());
			queue.Enqueue(e.Exception);

			GodotLog.Debug($"FirstChanceException: {e.Exception.GetType().Name}: " +
					$"'{e.Exception.Message}', osTid={osThreadId}, queueSize={queue.Count}");
		} finally {
			_isProcessing = false;
		}
	}

	protected override void OnEventSourceCreated(EventSource eventSource) {
		if (eventSource.Name == "Microsoft-Windows-DotNETRuntime") {
			EnableEvents(eventSource, EventLevel.Informational, (EventKeywords)ExceptionKeyword);
			GodotLog.Debug("Subscribed to Microsoft-Windows-DotNETRuntime exception events.");
		}
	}

	protected override void OnEventWritten(EventWrittenEventArgs eventData) {
		if (eventData.EventId == ExceptionCatchStart_EventId) {
			HandleExceptionCatchStart(eventData);
		}
	}

	private void HandleExceptionCatchStart(EventWrittenEventArgs eventData) {
		var methodName = eventData.Payload?[2] as string;
		var sourceOsTid = eventData.OSThreadId;

		try {
			_isProcessing = true;

			var queue = _threadExceptions.GetOrAdd(sourceOsTid,
					_ => new ConcurrentQueue<Exception>());

			GodotLog.Debug($"ExceptionCatchStart: method='{methodName}', " +
					$"sourceOsThreadId={sourceOsTid}, queueSize={queue.Count}");

			string? bridgeClass = methodName != null ? GetGodotBridgeClass(methodName) : null;

			if (bridgeClass != null && queue.TryDequeue(out var exception)) {
				GodotLog.Debug($"Capturing .NET exception caught by {bridgeClass}:\n  {exception.GetType().FullName}: \"{exception.Message}\"");
				exception.SetSentryMechanism(bridgeClass,
						"This exception was caught by the Godot engine bridge error handler. The application continued running.",
						handled: false);
				Sentry.SentrySdk.CaptureException(exception);
			} else if (bridgeClass != null) {
				GodotLog.Error($"Detected .NET exception caught by \"{bridgeClass}\", but no exception in queue for OS thread {sourceOsTid}. Ignored.");
			} else {
				// Caught by user code — dequeue and discard.
				queue.TryDequeue(out _);
				GodotLog.Debug($"Detected .NET exception that was handled by non-bridge code. Ignored.");
			}

			if (queue.IsEmpty) {
				_threadExceptions.TryRemove(sourceOsTid, out _);
			}
		} finally {
			_isProcessing = false;
		}
	}

	/// <summary>
	/// Returns the Godot bridge class name (e.g. "Godot.Bridge.CSharpInstanceBridge") if the
	/// method signature matches a known bridge catch site, or null if it's not a bridge method.
	/// </summary>
	private static string? GetGodotBridgeClass(string methodSignature) {
		if (!methodSignature.Contains("[GodotSharp]")) {
			return null;
		}

		foreach (var bridgeClass in _godotBridgeClasses) {
			if (methodSignature.Contains(bridgeClass)) {
				return bridgeClass.TrimEnd(':');
			}
		}
		return null;
	}

	public override void Dispose() {
		AppDomain.CurrentDomain.FirstChanceException -= OnFirstChanceException;
		base.Dispose();
	}
}
