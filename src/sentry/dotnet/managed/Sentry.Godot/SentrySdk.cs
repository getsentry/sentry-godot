using System;
using System.ComponentModel;
using System.Diagnostics;
using Sentry.Godot.ExceptionHandling;
using Sentry.Godot.Internal;
using Sentry.Godot.Interop;

namespace Sentry.Godot;

public static partial class SentrySdk
{
    static IDisposable? _exceptionHandler;

    static GodotAssemblyReader? _assemblyReader;

    // Re-entry guard for the Init() -> native init() -> InitFromNative() chain:
    // Init() triggers native initialization, which signals back into the .NET
    // layer via InitFromNative() and would otherwise run InitDotnet() a second
    // time.
    // Doesn't need locking, because the whole call chain runs on the same thread.
    static bool _initializing;

    // Re-entry guard for the Close() -> native close() -> CloseFromNative() chain:
    // Close() triggers native shutdown, which signals back into the .NET layer
    // via CloseFromNative() and would otherwise run CloseDotnetSdk() a second
    // time.
    // Doesn't need locking, because the whole call chain runs on the same thread.
    static bool _closing;

    [ThreadStatic] private static bool _inLocalScope;
    internal static bool InLocalScope => _inLocalScope;

    private readonly struct LocalScopeGuard : IDisposable
    {
        private readonly bool _prev;
        public LocalScopeGuard() { _prev = _inLocalScope; _inLocalScope = true; }
        public void Dispose() => _inLocalScope = _prev;
    }

    internal static SentryGodotOptions? CurrentOptions { get; private set; }

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
            // Fetch default attachments after native init resolves them.
            NativeBridge.FetchDefaultAttachments(godotOptions);
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
            NativeBridge.FetchDefaultAttachments(godotOptions);
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
        ConfigureAssemblyReader(godotOptions);
        Sentry.SentrySdk.Init(godotOptions);
        InitFirstChanceExceptionHandler();
    }

    /// <summary>
    /// On Android, lets the SDK read managed assembly bytes from the packed project data.
    /// </summary>
    /// <remarks>
    /// Godot's Mono runtime on Android loads managed assemblies from the packed
    /// project data, so they have no on-disk path and the SDK cannot build debug
    /// images for managed stack frames. The reader resolves the bytes through the
    /// native layer instead. Desktop builds keep the SDK's default disk reader,
    /// which finds the assemblies as loose files next to the executable.
    /// </remarks>
    private static void ConfigureAssemblyReader(SentryGodotOptions godotOptions)
    {
        if (!NativeBridge.IsAndroid() || godotOptions.AssemblyReader is not null)
        {
            return;
        }

        _assemblyReader = new GodotAssemblyReader();
        godotOptions.AssemblyReader = _assemblyReader.TryReadAssembly;
        GodotLog.Debug("Assembly reader registered for .NET stack symbolication.");
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
        if (_closing)
        {
            return;
        }
        _closing = true;
        try
        {
            CloseDotnetSdk();
            NativeBridge.CloseNativeSdk();
        }
        finally
        {
            _closing = false;
        }
    }

    internal static void CloseFromNative()
    {
        if (_closing)
        {
            return;
        }
        _closing = true;
        try
        {
            CloseDotnetSdk();
        }
        finally
        {
            _closing = false;
        }
    }

    private static void CloseDotnetSdk()
    {
        _exceptionHandler?.Dispose();
        _exceptionHandler = null;
        _assemblyReader = null;
        Sentry.SentrySdk.Close();
        CurrentOptions = null;
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

    /// <inheritdoc cref="M:Sentry.SentrySdk.CaptureEvent(Sentry.SentryEvent,System.Action{Sentry.Scope})"/>
    [DebuggerStepThrough]
    [EditorBrowsable(EditorBrowsableState.Never)]
    public static SentryId CaptureEvent(SentryEvent evt, Action<Scope> configureScope)
    {
        using var _ = new LocalScopeGuard();
        return Sentry.SentrySdk.CaptureEvent(evt, configureScope);
    }

    /// <inheritdoc cref="M:Sentry.SentrySdk.CaptureEvent(Sentry.SentryEvent,Sentry.SentryHint,System.Action{Sentry.Scope})"/>
    [DebuggerStepThrough]
    [EditorBrowsable(EditorBrowsableState.Never)]
    public static SentryId CaptureEvent(SentryEvent evt, SentryHint? hint, Action<Scope> configureScope)
    {
        using var _ = new LocalScopeGuard();
        return Sentry.SentrySdk.CaptureEvent(evt, hint, configureScope);
    }

    /// <inheritdoc cref="M:Sentry.SentrySdk.CaptureException(System.Exception,System.Action{Sentry.Scope})"/>
    [DebuggerStepThrough]
    public static SentryId CaptureException(Exception exception, Action<Scope> configureScope)
    {
        using var _ = new LocalScopeGuard();
        return Sentry.SentrySdk.CaptureException(exception, configureScope);
    }

    /// <inheritdoc cref="M:Sentry.HubExtensions.CaptureFeedback(Sentry.IHub,Sentry.SentryFeedback,System.Action{Sentry.Scope},Sentry.SentryHint)"/>
    [DebuggerStepThrough]
    public static SentryId CaptureFeedback(SentryFeedback feedback, Action<Scope> configureScope, SentryHint? hint = null)
    {
        using var _ = new LocalScopeGuard();
        return Sentry.SentrySdk.CaptureFeedback(feedback, configureScope, hint);
    }

    /// <inheritdoc cref="M:Sentry.IHub.CaptureFeedback(Sentry.SentryFeedback,Sentry.CaptureFeedbackResult@,System.Action{Sentry.Scope},Sentry.SentryHint)"/>
    [DebuggerStepThrough]
    public static SentryId CaptureFeedback(SentryFeedback feedback, out CaptureFeedbackResult result,
        Action<Scope> configureScope, SentryHint? hint = null)
    {
        using var _ = new LocalScopeGuard();
        return Sentry.SentrySdk.CaptureFeedback(feedback, out result, configureScope, hint);
    }

    /// <inheritdoc cref="M:Sentry.SentrySdk.CaptureMessage(System.String,System.Action{Sentry.Scope},Sentry.SentryLevel)"/>
    [DebuggerStepThrough]
    public static SentryId CaptureMessage(string message, Action<Scope> configureScope, SentryLevel level = SentryLevel.Info)
    {
        using var _ = new LocalScopeGuard();
        return Sentry.SentrySdk.CaptureMessage(message, configureScope, level);
    }
}
