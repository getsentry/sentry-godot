using System;
using System.Collections.Generic;
using System.Diagnostics.Tracing;
using System.Runtime.ExceptionServices;
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
/// A FIFO queue per thread is needed because multiple FirstChanceException events
/// fire synchronously before the EventListener processes any ExceptionCatchStart.
/// </summary>
internal class CoreClrExceptionHandler : EventListener
{
    private const int ExceptionCatchStart_EventId = 250;
    private const long ExceptionKeyword = 0x8000;
    private const int MaxQueueExceptionsPerThread = 32;

    // Godot bridge catch sites: IL signature prefix -> display name.
    // ExceptionCatchStart passes MethodName containing full IL signature, e.g.:
    //   "valuetype ... [GodotSharp] Godot.Bridge.CSharpInstanceBridge::Call(...)"
    private static readonly (string Prefix, string DisplayName)[] _godotBridgeMethods = {
        ("Godot.Bridge.CSharpInstanceBridge::", "Godot.Bridge.CSharpInstanceBridge"),
        ("Godot.Bridge.ScriptManagerBridge::", "Godot.Bridge.ScriptManagerBridge"),
        ("Godot.DelegateUtils::", "Godot.DelegateUtils"),
        ("Godot.SignalAwaiter::", "Godot.SignalAwaiter"),
        ("Godot.GD::", "Godot.GD"),
    };

    // Single lock guards both _threadExceptions and the per-thread queues.
    private readonly object _lock = new();
    private readonly Dictionary<long, Queue<(DateTime Timestamp, Exception Exception)>> _threadExceptions = [];

    // Prevents re-entrancy when CaptureException itself throws (e.g. serialization).
    // ThreadStatic so the EventListener thread and throwing thread are guarded independently.
    [ThreadStatic]
    private static bool _isProcessing;

    public CoreClrExceptionHandler()
    {
        AppDomain.CurrentDomain.FirstChanceException += OnFirstChanceException;
    }

    private void OnFirstChanceException(object? sender, FirstChanceExceptionEventArgs e)
    {
        if (_isProcessing)
        {
            return;
        }

        try
        {
            _isProcessing = true;

            long osThreadId = NativeThreadId.GetCurrentThreadId();
            var timestamp = DateTime.UtcNow;
            lock (_lock)
            {
                if (!_threadExceptions.TryGetValue(osThreadId, out var queue))
                {
                    queue = [];
                    _threadExceptions[osThreadId] = queue;
                }
                queue.Enqueue((timestamp, e.Exception));
                if (queue.Count > MaxQueueExceptionsPerThread)
                {
                    queue.Dequeue();
                }
            }
        }
        finally
        {
            _isProcessing = false;
        }
    }

    protected override void OnEventSourceCreated(EventSource eventSource)
    {
        if (eventSource.Name == "Microsoft-Windows-DotNETRuntime")
        {
            EnableEvents(eventSource, EventLevel.Informational, (EventKeywords)ExceptionKeyword);
            GodotLog.Debug("Subscribed to Microsoft-Windows-DotNETRuntime exception events.");
        }
    }

    protected override void OnEventWritten(EventWrittenEventArgs eventData)
    {
        if (eventData.EventId == ExceptionCatchStart_EventId)
        {
            HandleExceptionCatchStart(eventData);
        }
    }

    private void HandleExceptionCatchStart(EventWrittenEventArgs eventData)
    {
        var sourceOsTid = eventData.OSThreadId;
        var catchTimestamp = eventData.TimeStamp;
        string? bridgeName = eventData.Payload?[2] is string methodName ? GetGodotBridgeName(methodName) : null;

        try
        {
            _isProcessing = true;

            Exception? matched = null;
            int remainingInQueue = 0;

            lock (_lock)
            {
                // Invariant: queues in _threadExceptions are always non-empty.
                if (_threadExceptions.TryGetValue(sourceOsTid, out var queue))
                {
                    // Match most recent exception recorded on this thread at or before the catch event,
                    // and remove it and any older entries from the queue.
                    while (queue.Count > 0 && queue.Peek().Timestamp <= catchTimestamp)
                    {
                        matched = queue.Dequeue().Exception;
                    }

                    if (queue.Count == 0)
                    {
                        _threadExceptions.Remove(sourceOsTid);
                    }

                    remainingInQueue = queue.Count;
                }
            }

            if (matched is not null && bridgeName is not null)
            {
                GodotLog.Debug($"Capturing .NET exception caught by {bridgeName} (queue={remainingInQueue}):\n   {matched.GetType().FullName}: \"{matched.Message}\"");
                matched.SetSentryMechanism(bridgeName,
                        "This exception was caught by the Godot engine bridge error handler. The application continued running.",
                        handled: false);
                Sentry.SentrySdk.CaptureException(matched);
            }
        }
        finally
        {
            _isProcessing = false;
        }
    }

    private const string GodotSharpMarker = "[GodotSharp] ";

    /// <summary>
    /// Returns the display name of the Godot bridge class if the method signature matches
    /// a known bridge catch site, or null if it's not a bridge method.
    /// </summary>
    private static string? GetGodotBridgeName(string methodSignature)
    {
        ReadOnlySpan<char> span = methodSignature;
        var idx = span.IndexOf(GodotSharpMarker);
        if (idx < 0)
        {
            return null;
        }

        var afterMarker = span[(idx + GodotSharpMarker.Length)..];
        foreach (var (prefix, displayName) in _godotBridgeMethods)
        {
            if (afterMarker.StartsWith(prefix))
            {
                return displayName;
            }
        }
        return null;
    }

    public override void Dispose()
    {
        AppDomain.CurrentDomain.FirstChanceException -= OnFirstChanceException;
        base.Dispose();
    }
}
