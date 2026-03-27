using System;
using System.Runtime.ExceptionServices;
using Godot;
using Sentry;
using Sentry.Godot.Internal;

namespace Sentry.Godot.ExceptionHandling;

/// <summary>
/// Uses FirstChanceException to capture the last exception per thread,
/// and Godot Logger to detect when Godot's bridge catches one.
/// When _LogError fires with C# stack frames, we capture the cached exception.
/// </summary>
/// <remarks>
/// Note that, Godot doesn't log .NET exceptions when its debugger is active,
/// so this handler alone is not sufficient on desktop platforms during debugging.
/// That's why CoreClrExceptionHandler also exists - it captures exceptions even
/// with the debugger attached, but only on CoreCLR (PC/Mac).
/// </remarks>
public class LoggerExceptionHandler : IDisposable {
	// Last exception per thread. Both FirstChanceException and _LogError run on the
	// same thread, so [ThreadStatic] is sufficient — no cross-thread synchronization needed.
	// Only the most recent exception matters: _LogError always wants the last one thrown
	// (the one the bridge caught), so a single slot replaces the previous drain-to-last queue.
	[ThreadStatic]
	private static Exception? _lastException;

	// Re-entrancy guard per thread.
	[ThreadStatic]
	private static bool _isProcessing;

	private readonly SentryLogger _logger;

	public LoggerExceptionHandler() {
		AppDomain.CurrentDomain.FirstChanceException += OnFirstChanceException;
		_logger = new SentryLogger(this);
		OS.AddLogger(_logger);
		GodotLog.Debug("Registered Logger-based exception handler.");
	}

	private void OnFirstChanceException(object? sender, FirstChanceExceptionEventArgs e) {
		if (!_isProcessing) {
			_lastException = e.Exception;
		}
	}

	internal void OnLogError(string file, string code) {
		if (_isProcessing || _lastException == null) {
			return;
		}

		// Validate that _LogError was triggered by _lastException, not a stale one.
		// The bridge passes the exception type name (and possibly message) as the "code" parameter.
		var expectedType = _lastException.GetType().FullName;
		if (expectedType == null || code == null || !code.StartsWith(expectedType, StringComparison.Ordinal)) {
			// Not triggered by the current exception - ignore.
			return;
		}

		try {
			_isProcessing = true;

			var exception = _lastException!;
			_lastException = null;

			GodotLog.Debug($"Capturing .NET exception caught by Godot bridge:\n  {exception.GetType().FullName}: \"{exception.Message}\"");
			exception.SetSentryMechanism("Godot.Bridge",
					"This exception was caught by the Godot engine bridge error handler. The application continued running.",
					handled: false);
			Sentry.SentrySdk.CaptureException(exception);
		} finally {
			_isProcessing = false;
		}
	}

	public void Dispose() {
		AppDomain.CurrentDomain.FirstChanceException -= OnFirstChanceException;
	}
}

/// <summary>
/// Godot Logger that forwards C# error events to LoggerExceptionHandler.
/// This helps to flag exceptions as unhandled by the user code and capture them.
/// </summary>
public partial class SentryLogger : global::Godot.Logger {
	private readonly LoggerExceptionHandler _handler;

	public SentryLogger(LoggerExceptionHandler handler) {
		_handler = handler;
	}

	public override void _LogError(string function, string file, int line, string code,
			string rationale, bool editorNotify, int errorType,
			global::Godot.Collections.Array<ScriptBacktrace> scriptBacktraces) {
		GodotLog.Debug($"LogError: function={function}, file={file}, line={line}, code={code}, rationale={rationale}, errorType={errorType}");
		_handler.OnLogError(file, code);
	}
}
