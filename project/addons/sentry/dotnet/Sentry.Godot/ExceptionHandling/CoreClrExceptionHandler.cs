using System;
using System.Collections.Generic;
using System.Diagnostics.Tracing;
using System.Runtime.ExceptionServices;
using Sentry.Godot.Internal;

namespace Sentry.Godot.ExceptionHandling;

// TODO: remove debug prints when PR is ready

/// <summary>
/// Collects managed exceptions via FirstChanceException and keeps them in a
/// per-thread FIFO queue for later correlation with CLR catch events.
/// </summary>
/// <remarks>
/// Thread-safety: OnFirstChanceException runs on the throwing thread;
/// DrainPending() runs on the listener's thread and guarded by lock.
///
/// Capture scope: consumers wrap their capture call in EnterCaptureScope().
/// Any FirstChanceException that fires on the same thread inside that scope is
/// dropped, preventing feedback loops if the capture path itself throws.
/// </remarks>
internal class FirstChanceExceptionPool : IDisposable
{
    // Prevents collecting exceptions from listener thread if it throws.
    [ThreadStatic]
    private static bool _inCaptureScope;

    internal sealed class CaptureScopeToken : IDisposable
    {
        public void Dispose() => _inCaptureScope = false;
    }

    private static readonly CaptureScopeToken _statelessCaptureToken = new();

    public static IDisposable EnterCaptureScope()
    {
        _inCaptureScope = true;
        return _statelessCaptureToken; // safe to reuse
    }

    private readonly object _pendingLock = new();
    private readonly Dictionary<long, Queue<Exception>> _pending = []; // OSThreadId => pending exceptions

    public FirstChanceExceptionPool()
    {
        AppDomain.CurrentDomain.FirstChanceException += OnFirstChanceException;
    }

    private void OnFirstChanceException(object? sender, FirstChanceExceptionEventArgs e)
    {
        if (_inCaptureScope)
        {
            return;
        }

        long osThreadId = NativeThreadId.GetCurrentThreadId();
        GodotLog.Debug($"[diag] FCE tid={osThreadId} type={e.Exception.GetType().Name}");

        lock (_pendingLock)
        {
            if (!_pending.TryGetValue(osThreadId, out var p))
            {
                p = [];
                _pending[osThreadId] = p;
            }
            p.Enqueue(e.Exception);
        }
    }

    public Exception? DrainPending(long threadId, int drainCount)
    {
        lock (_pendingLock)
        {
            if (!_pending.TryGetValue(threadId, out var list) || list.Count < drainCount)
            {
                return null;
            }

            Exception matched;
            do
            {
                matched = list.Dequeue();
            } while (--drainCount > 0);

            if (list.Count == 0)
            {
                _pending.Remove(threadId);
            }

            return matched;
        }
    }

    public void Dispose()
    {
        AppDomain.CurrentDomain.FirstChanceException -= OnFirstChanceException;
    }
}

/// <summary>
/// Detects "unhandled" C# exceptions in Godot by combining FirstChanceException
/// with .NET runtime EventSource events. When ExceptionCatchStart reports a
/// Godot bridge method as the catcher, the exception is classified as unhandled
/// by user code and sent to Sentry.
/// </summary>
/// <remarks>
/// Key insight: FirstChanceException runs on the throwing thread, but
/// EventListener callbacks run on a dedicated background thread. We correlate
/// via OS thread ID. A FIFO queue per thread is needed because multiple
/// FirstChanceException events fire synchronously before the EventListener
/// processes any ExceptionCatchStart(250).
///
/// ExceptionThrown(80) is emitted before FirstChanceException, but their
/// arrival order on the listener side is racey, so we must not depend on it.
/// The only reliable ordering here is that both arrive before
/// ExceptionCatchStart(250).
///
/// See: https://learn.microsoft.com/en-us/dotnet/fundamentals/diagnostics/runtime-exception-events
/// </remarks>
internal class CoreClrExceptionHandler : EventListener
{
    private const int ExceptionThrown_EventId = 80;
    private const int ExceptionCatchStart_EventId = 250;
    private const long ExceptionKeyword = 0x8000; // bitmask to target exception events

    private readonly FirstChanceExceptionPool _exceptionsPool = new();

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

    private class ThreadContext
    {
        public int throwCount;
    }
    private readonly Dictionary<long, ThreadContext> _threadContexts = []; // only listener thread

    protected override void OnEventSourceCreated(EventSource eventSource)
    {
        if (eventSource.Name == "Microsoft-Windows-DotNETRuntime")
        {
            EnableEvents(eventSource, EventLevel.Informational, (EventKeywords)ExceptionKeyword);
            GodotLog.Debug("Subscribed to Microsoft-Windows-DotNETRuntime events.");
        }
        else if (eventSource.Name == "Sentry-Godot-Exceptions")
        {
            EnableEvents(eventSource, EventLevel.Informational);
            GodotLog.Debug("Subscribed to Sentry-Godot-Exceptions events.");
        }
    }

    protected override void OnEventWritten(EventWrittenEventArgs eventData)
    {
        if (eventData.OSThreadId == NativeThreadId.GetCurrentThreadId())
        {
            // Ignore events from the listener thread to avoid recursions.
            return;
        }

        switch (eventData.EventId)
        {
            case ExceptionThrown_EventId:
                HandleExceptionThrown(eventData);
                break;
            case ExceptionCatchStart_EventId:
                HandleExceptionCatchStart(eventData);
                break;
        }
    }


    /// <remarks>
    /// Runs on the listener thread.
    /// </remarks>
    private void HandleExceptionThrown(EventWrittenEventArgs eventData)
    {
        var tid = eventData.OSThreadId;
        GodotLog.Debug($"[diag] THROW srcTid={tid} handlerTid={NativeThreadId.GetCurrentThreadId()} throwTs={eventData.TimeStamp:HH:mm:ss.fffffff}");

        // Track how many exceptions have been thrown before catch.
        if (!_threadContexts.TryGetValue(tid, out var ctx))
        {
            ctx = new ThreadContext { throwCount = 0 };
            _threadContexts[tid] = ctx;
        }
        ctx.throwCount++;
    }

    /// <remarks>
    /// Runs on the listener thread.
    /// </remarks>
    private void HandleExceptionCatchStart(EventWrittenEventArgs eventData)
    {
        using var scopeGuard = FirstChanceExceptionPool.EnterCaptureScope();
        try
        {
            var sourceThreadId = eventData.OSThreadId;
            string? bridgeName = eventData.Payload?[2] is string methodName ? GetGodotBridgeName(methodName) : null;

            GodotLog.Debug($"[diag] CATCH srcTid={sourceThreadId} handlerTid={NativeThreadId.GetCurrentThreadId()} ts={eventData.TimeStamp:HH:mm:ss.fffffff} method={eventData.Payload?[2] as string ?? "<null>"}");

            if (!_threadContexts.TryGetValue(sourceThreadId, out var ctx))
            {
                ctx = new ThreadContext { throwCount = 0 };
                _threadContexts[sourceThreadId] = ctx;
            }

            var drainCount = Math.Max(ctx.throwCount, 1);
            Exception? matched = _exceptionsPool.DrainPending(sourceThreadId, drainCount);
            ctx.throwCount = 0;

            GodotLog.Debug($"[diag] match tid={sourceThreadId} bridge={bridgeName} matched={matched?.GetType().Name ?? "<null>"}");

            if (matched is not null && bridgeName is not null)
            {
                GodotLog.Debug($"Capturing .NET exception caught by {bridgeName}:\n   {matched.GetType().FullName}: \"{matched.Message}\"");
                matched.SetSentryMechanism(bridgeName,
                        "This exception was caught by the Godot engine bridge error handler. The application continued running.",
                        handled: false);
                Sentry.SentrySdk.CaptureException(matched);
            }
        }
        catch (Exception ex)
        {
            GodotLog.Error("CoreClrExceptionHandler failed to capture exception due to: " + ex);
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
        _exceptionsPool.Dispose();
        base.Dispose();
    }
}
