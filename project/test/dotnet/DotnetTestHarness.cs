using System;
using Godot;
using Sentry;
using Sentry.Godot;

/// <summary>
/// Drives the .NET layer for the CPP integration testing (doctest).
/// See /tests/cpp/.
/// </summary>
public partial class DotnetTestHarness : RefCounted
{
    public void Init()
    {
        SentrySdk.Init(options =>
        {
            options.Debug = false;
            options.AttachScreenshot = false; // CI runs headless
        });
    }

    public void InitWithNativeHooks()
    {
        SentrySdk.Init(options =>
        {
            options.Debug = false;
            options.AttachScreenshot = false; // CI runs headless
            options.Native.SetBeforeSend(OnBeforeSend);
            options.SetBeforeSend((SentryEvent _) => null);
        });
    }

    /// <summary>
    /// Captures a managed event, running the .NET event processors that reach into the native layer.
    /// Returns false if the managed SDK is disabled, meaning nothing was captured.
    /// </summary>
    public bool CaptureManagedEvent()
    {
        if (!SentrySdk.IsEnabled)
        {
            return false;
        }

        SentrySdk.CaptureMessage("Managed event from the CPP test harness");
        return true;
    }

    public void Close()
    {
        SentrySdk.Close();
    }

    private readonly Godot.Collections.Dictionary _receivedOptions = [];
    public Godot.Collections.Dictionary GetReceivedOptions() => _receivedOptions;

    /// <summary>
    /// Records every option the native layer handed over, then writes a different value to each one.
    /// The CPP test compares the recorded values against the project settings it wrote, and the values
    /// written here against the options the native layer initialized with, covering both structs.
    /// </summary>
    public void InitWithInteropOptions()
    {
        SentrySdk.Init(options =>
        {
            // These arrive from the native layer, so they show what NativeOptions carried over.
            // Keyed by option name, and durations report as whole milliseconds, both so the CPP
            // side can look them up straight from its option table.
            _receivedOptions["dsn"] = options.Dsn;
            _receivedOptions["release"] = options.Release;
            _receivedOptions["dist"] = options.Distribution;
            _receivedOptions["environment"] = options.Environment;
            _receivedOptions["debug"] = options.Debug;
            _receivedOptions["diagnostic_level"] = (int)options.DiagnosticLevel;
            _receivedOptions["sample_rate"] = options.SampleRate ?? -1.0f;
            _receivedOptions["traces_sample_rate"] = options.TracesSampleRate ?? -1.0;
            _receivedOptions["max_breadcrumbs"] = options.MaxBreadcrumbs;
            _receivedOptions["shutdown_timeout_ms"] = (int)options.ShutdownTimeout.TotalMilliseconds;
            _receivedOptions["send_default_pii"] = options.SendDefaultPii;
            _receivedOptions["attach_log"] = options.AttachLog;
            _receivedOptions["attach_scene_tree"] = options.AttachSceneTree;
            _receivedOptions["attach_screenshot"] = options.AttachScreenshot;
            _receivedOptions["screenshot_level"] = (int)options.ScreenshotLevel;
            _receivedOptions["enable_app_hang_tracking"] = options.EnableAppHangTracking;
            _receivedOptions["app_hang_timeout_ms"] = (int)options.AppHangTimeout.TotalMilliseconds;
            _receivedOptions["enabled"] = options.GodotLogger.Enabled;
            _receivedOptions["include_source_context"] = options.GodotLogger.IncludeSourceContext;
            _receivedOptions["include_variables"] = options.GodotLogger.IncludeVariables;
            _receivedOptions["event_mask"] = (int)options.GodotLogger.EventMask;
            _receivedOptions["breadcrumb_mask"] = (int)options.GodotLogger.BreadcrumbMask;
            _receivedOptions["log_mask"] = (int)options.GodotLogger.LogMask;
            _receivedOptions["events_per_frame"] = options.GodotLogger.Limits.EventsPerFrame;
            _receivedOptions["repeated_error_window_ms"] = (int)options.GodotLogger.Limits.RepeatedErrorWindow.TotalMilliseconds;
            _receivedOptions["throttle_events"] = options.GodotLogger.Limits.ThrottleEvents;
            _receivedOptions["throttle_window_ms"] = (int)options.GodotLogger.Limits.ThrottleWindow.TotalMilliseconds;
            _receivedOptions["enable_anr_detection"] = options.Android.EnableAnrDetection;
            _receivedOptions["anr_timeout_interval_ms"] = (int)options.Android.AnrTimeoutInterval.TotalMilliseconds;
            _receivedOptions["attach_anr_thread_dump"] = options.Android.AttachAnrThreadDump;

            // Init sends these back through ManagedOptions. Every value differs from the one above,
            // so a value can only reach the native layer by actually crossing the boundary.
            options.Dsn = "https://bbb222@127.0.0.1/22";
            options.Release = "interop-managed-release@2.2.2";
            options.Distribution = "interop-managed-dist";
            options.Environment = "interop-managed-env";
            options.Debug = false;
            options.DiagnosticLevel = SentryLevel.Error;
            options.SampleRate = 0.75f;
            options.TracesSampleRate = 0.8;
            options.MaxBreadcrumbs = 22;
            options.ShutdownTimeout = TimeSpan.FromMilliseconds(2200);
            options.SendDefaultPii = false;
            options.AttachLog = true;
            options.AttachSceneTree = false;
            options.AttachScreenshot = false;
            options.ScreenshotLevel = SentryLevel.Warning;
            options.EnableAppHangTracking = true;
            options.AppHangTimeout = TimeSpan.FromMilliseconds(2300);
            options.GodotLogger.Enabled = true;
            options.GodotLogger.IncludeSourceContext = true;
            options.GodotLogger.IncludeVariables = false;
            options.GodotLogger.EventMask = GodotLoggerEventMask.Script | GodotLoggerEventMask.Shader;
            options.GodotLogger.BreadcrumbMask = GodotLoggerEventMask.Warning;
            options.GodotLogger.LogMask = GodotLoggerEventMask.Error;
            options.GodotLogger.Limits.EventsPerFrame = 21;
            options.GodotLogger.Limits.RepeatedErrorWindow = TimeSpan.FromMilliseconds(2201);
            options.GodotLogger.Limits.ThrottleEvents = 22;
            options.GodotLogger.Limits.ThrottleWindow = TimeSpan.FromMilliseconds(2202);
            options.Android.EnableAnrDetection = true;
            options.Android.AnrTimeoutInterval = TimeSpan.FromMilliseconds(2400);
            options.Android.AttachAnrThreadDump = false;
        });
    }

    public string GetCurrentTraceId()
    {
        return SentrySdk.GetTraceHeader()?.TraceId.ToString() ?? "";
    }

    private ITransactionTracer _activeTransaction;

    public string StartTransaction(string name, string operation)
    {
        _activeTransaction = SentrySdk.StartTransaction(name, operation);
        SentrySdk.ConfigureScope(scope => scope.Transaction = _activeTransaction);
        return _activeTransaction.TraceId.ToString();
    }

    public string FinishTransaction()
    {
        _activeTransaction?.Finish();
        _activeTransaction = null;
        return GetCurrentTraceId();
    }

    private readonly Godot.Collections.Dictionary _seenEventValues = [];
    public Godot.Collections.Dictionary GetSeenEventValues() => _seenEventValues;

    private int _nativeBeforeSendCallCount;
    public int GetNativeBeforeSendCallCount() => _nativeBeforeSendCallCount;

    /// <summary>
    /// Native before-send callback exercised by the CPP tests.
    /// Records the values it reads through the getters, then overrides them through the setters.
    /// </summary>
    private SentryNativeEvent OnBeforeSend(SentryNativeEvent ev)
    {
        _nativeBeforeSendCallCount++;

        if (ev.Message is not null && ev.Message.Contains("DROP_ME"))
        {
            return null;
        }

        _seenEventValues["message"] = ev.Message;
        _seenEventValues["level"] = ev.Level.ToString();
        _seenEventValues["release"] = ev.Release;
        _seenEventValues["environment"] = ev.Environment;
        _seenEventValues["logger"] = ev.Logger;
        _seenEventValues["tag"] = ev.GetTag("before_send.read_me");

        ev.Message = "Before-send override: 世界 👋";
        ev.Level = SentryLevel.Warning;
        ev.SetTag("before_send.added", "added 世界 👋");
        ev.UnsetTag("before_send.remove_me");
        return ev;
    }
}
