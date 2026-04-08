using System;
using System.Runtime.ExceptionServices;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot.ExceptionHandling;

/// <summary>
/// Uses FirstChanceException to capture the last exception per thread,
/// and native Godot Logger to detect when Godot's bridge catches one.
/// When the native logger detects C# error, we capture the cached exception.
/// </summary>
/// <remarks>
/// Note that, Godot doesn't log .NET exceptions when its debugger is active,
/// so this handler alone is not sufficient on desktop platforms during debugging.
/// That's why CoreClrExceptionHandler also exists - it captures exceptions even
/// with the debugger attached, but only on CoreCLR (PC/Mac).
/// </remarks>
internal class LoggerExceptionHandler : IDisposable
{
    // Last exception per thread. Both FirstChanceException and the native logger callback
    // run on the same thread, so [ThreadStatic] should be enough - no synchronization needed.
    [ThreadStatic]
    private static Exception? _lastException;

    // Re-entrancy guard per thread.
    [ThreadStatic]
    private static bool _isProcessing;

    public LoggerExceptionHandler()
    {
        AppDomain.CurrentDomain.FirstChanceException += OnFirstChanceException;
        NativeBridge.RegisterLoggerErrorHandler(OnLogError);
        GodotLog.Debug("Registered Logger-based exception handler.");
    }

    private static void OnFirstChanceException(object? sender, FirstChanceExceptionEventArgs e)
    {
        if (!_isProcessing)
        {
            _lastException = e.Exception;
        }
    }

    private static void OnLogError(string file, string code)
    {
        if (_isProcessing || _lastException == null)
        {
            return;
        }

        // Validate that _LogError was triggered by _lastException, not a stale one.
        // The bridge passes the exception type name (and possibly message) as the "code" parameter.
        var expectedType = _lastException.GetType().FullName;
        if (expectedType == null || code == null || !code.StartsWith(expectedType, StringComparison.Ordinal))
        {
            // Not triggered by the current exception - ignore.
            return;
        }

        try
        {
            _isProcessing = true;

            var exception = _lastException!;
            _lastException = null;

            GodotLog.Debug($"Capturing .NET exception caught by Godot bridge:\n  {exception.GetType().FullName}: \"{exception.Message}\"");
            exception.SetSentryMechanism("Godot.Bridge",
                    "This exception was caught by the Godot engine bridge error handler. The application continued running.",
                    handled: false);
            Sentry.SentrySdk.CaptureException(exception);
        }
        finally
        {
            _isProcessing = false;
        }
    }

    public void Dispose()
    {
        AppDomain.CurrentDomain.FirstChanceException -= OnFirstChanceException;
    }
}
