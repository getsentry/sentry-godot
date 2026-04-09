using System;
using Sentry.Godot.ExceptionHandling;
using Sentry.Godot.Internal;
using Sentry.Godot.Interop;

namespace Sentry.Godot;

public class SentrySdk
{
    static IDisposable? _exceptionHandler;
    static bool _initializing;
    internal static SentryGodotOptions? CurrentOptions { get; private set; }

    public static bool IsEnabled => Sentry.SentrySdk.IsEnabled;

    /// <summary>
    /// Initializes the .NET SDK with an optional configuration callback.
    /// </summary>
    public static void Init(Action<SentryGodotOptions>? configureOptions = null)
    {
        if (_initializing)
        {
            return;
        }
        if (IsEnabled)
        {
            GodotLog.Error("Sentry .NET SDK is already initialized. Ignoring Init() call.");
            return;
        }
        _initializing = true;
        try
        {
            var godotOptions = new SentryGodotOptions();
            godotOptions.ApplyNativeOptionsDefaults();
            configureOptions?.Invoke(godotOptions);
            godotOptions.ApplyTemplateSubstitutions();

            // Use the same order as in automatic initialization for consistency.
            InitNativeIfNeeded(godotOptions);
            InitDotnet(godotOptions);
        }
        finally
        {
            _initializing = false;
        }
    }

    /// <summary>
    /// Initializes the .NET SDK using resolved options from the native layer.
    /// </summary>
    /// <remarks>
    /// Always called by the native SDK after initialization, but should only take
    /// effect when the native layer is auto-initialized or initialized from GDScript.
    /// </remarks>
    internal static void InitFromNative()
    {
        if (_initializing)
        {
            return;
        }
        _initializing = true;
        try
        {
            var godotOptions = new SentryGodotOptions();
            godotOptions.ApplyNativeOptions();
            godotOptions.ApplyTemplateSubstitutions();
            InitDotnet(godotOptions);
        }
        finally
        {
            _initializing = false;
        }
    }

    private static void InitDotnet(SentryGodotOptions godotOptions)
    {
        CurrentOptions = godotOptions;
        GodotLog.Debug("Initializing Sentry in .NET...");
        Sentry.SentrySdk.Init(godotOptions);
        InitFirstChanceExceptionHandler();
    }

    /// <summary>
    /// If the native SDK hasn't initialized yet (manual init case),
    /// trigger native initialization via P/Invoke.
    /// </summary>
    private static void InitNativeIfNeeded(SentryGodotOptions godotOptions)
    {
        if (NativeBridge.IsEnabled())
        {
            return;
        }
        NativeBridge.InitNativeSdk(godotOptions);
    }

    /// <summary>
    /// Flushes the events and disables the SDK.
    /// </summary>
    public static void Close()
    {
        _exceptionHandler?.Dispose();
        _exceptionHandler = null;
        CurrentOptions = null;
        Sentry.SentrySdk.Close();
        NativeBridge.CloseNativeSdk();
    }

    private static void InitFirstChanceExceptionHandler()
    {
        if (_exceptionHandler is not null)
        {
            return;
        }

        // Both handlers use FirstChanceException to intercept exceptions caught by
        // Godot's bridge in try-catch blocks. They differ in how they detect the catch.
        if (NativeBridge.IsDebuggerActive())
        {
            // During debugger sessions (projects played from editor), Godot suppresses
            // error logging, so LoggerExceptionHandler can't detect bridge catches.
            // CoreClrExceptionHandler uses EventListener events instead - works with
            // debugger but has more overhead and only supports CoreCLR (i.e. PC and Mac).
            GodotLog.Debug("Debugger active - using EventListener-based exception handler.");
            if (NativeBridge.IsAndroid())
            {
                GodotLog.Warn("Can't capture exceptions in .NET layer on Android while Godot debugger is active.");
            }
            _exceptionHandler = new CoreClrExceptionHandler();
        }
        else
        {
            // LoggerExceptionHandler uses Godot's Logger to detect bridge catches.
            // More efficient and works on all runtimes (CoreCLR, MonoVM, NativeAOT).
            GodotLog.Debug("Using Logger-based exception handler.");
            _exceptionHandler = new LoggerExceptionHandler();
        }
        GodotLog.Debug(".NET first chance exception handler initialized.");
    }
}
