using System;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
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
            options.Native.SetBeforeSendTransaction(OnBeforeSendTransaction);
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

    public void StartSession() => SentrySdk.StartSession();

    public void EndSession() => SentrySdk.EndSession();

#pragma warning disable SENTRYGD1001 // Verify that Godot session control does not create a managed session.
    public bool IsManagedSessionActive() => Sentry.SentrySdk.IsSessionActive;
#pragma warning restore SENTRYGD1001

    private readonly Godot.Collections.Dictionary _receivedOptions = [];
    public Godot.Collections.Dictionary GetReceivedOptions() => _receivedOptions;

    public void InitWithRegexTracePropagationTargets()
    {
        SentrySdk.Init(options =>
        {
            options.Debug = false;
            options.AttachScreenshot = false;
            options.TracePropagationTargets.Clear();
            options.TracePropagationTargets.Add(@"^literal\.example\.com$");
            options.TracePropagationTargets.Add(new Regex(@"^https://api\.example\.com/żółw$"));
            options.TracePropagationTargets.Add("");
            options.TracePropagationTargets.Add(new Regex(""));
        });
    }

    public void InitWithEmptyTracePropagationTargets()
    {
        SentrySdk.Init(options =>
        {
            options.Debug = false;
            options.AttachScreenshot = false;
            options.TracePropagationTargets.Clear();
        });
    }

    public string[] GetCurrentTracePropagationTargets()
    {
        var optionsProperty = typeof(SentrySdk).GetProperty("CurrentOptions", BindingFlags.NonPublic | BindingFlags.Static);
        if (optionsProperty?.GetValue(null) is not SentryGodotOptions options)
        {
            return [];
        }

        var targets = new string[options.TracePropagationTargets.Count];
        for (int i = 0; i < targets.Length; i++)
        {
            var target = options.TracePropagationTargets[i];
            string type = target.IsRegex ? "regex" : "string";
            targets[i] = $"{type}:{target}";
        }
        return targets;
    }

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
            var tracePropagationTargets = new string[options.TracePropagationTargets.Count];
            for (int i = 0; i < tracePropagationTargets.Length; i++)
            {
                tracePropagationTargets[i] = options.TracePropagationTargets[i].ToString();
            }
            _receivedOptions["trace_propagation_targets"] = tracePropagationTargets;
            _receivedOptions["propagate_traceparent"] = options.PropagateTraceparent;
            _receivedOptions["org_id"] = options.OrgId;
            _receivedOptions["auto_session_tracking"] = options.AutoSessionTracking;
            _receivedOptions["session_tracking_interval_ms"] = (int)options.AutoSessionTrackingInterval.TotalMilliseconds;
            _receivedOptions["max_breadcrumbs"] = options.MaxBreadcrumbs;
            _receivedOptions["shutdown_timeout_ms"] = (int)options.ShutdownTimeout.TotalMilliseconds;
            _receivedOptions["send_default_pii"] = options.SendDefaultPii;
            _receivedOptions["attach_log"] = options.AttachLog;
            _receivedOptions["attach_scene_tree"] = options.AttachSceneTree;
            _receivedOptions["attach_screenshot"] = options.AttachScreenshot;
            _receivedOptions["screenshot_level"] = (int)options.ScreenshotLevel;
            _receivedOptions["enable_app_hang_tracking"] = options.EnableAppHangTracking;
            _receivedOptions["app_hang_timeout_ms"] = (int)options.AppHangTimeout.TotalMilliseconds;
            _receivedOptions["auto_metrics:enable_frame_metrics"] = options.AutoMetrics.EnableFrameMetrics;
            _receivedOptions["auto_metrics:frame_metrics_interval_sec"] = options.AutoMetrics.FrameMetricsIntervalSec;
            _receivedOptions["auto_metrics:enable_rendering_metrics"] = options.AutoMetrics.EnableRenderingMetrics;
            _receivedOptions["auto_metrics:rendering_metrics_interval_sec"] = options.AutoMetrics.RenderingMetricsIntervalSec;
            _receivedOptions["auto_metrics:enable_memory_metrics"] = options.AutoMetrics.EnableMemoryMetrics;
            _receivedOptions["auto_metrics:memory_metrics_interval_sec"] = options.AutoMetrics.MemoryMetricsIntervalSec;
            _receivedOptions["auto_metrics:enable_network_metrics"] = options.AutoMetrics.EnableNetworkMetrics;
            _receivedOptions["auto_metrics:network_metrics_interval_sec"] = options.AutoMetrics.NetworkMetricsIntervalSec;
            _receivedOptions["godot_logger:enabled"] = options.GodotLogger.Enabled;
            _receivedOptions["godot_logger:include_source_context"] = options.GodotLogger.IncludeSourceContext;
            _receivedOptions["godot_logger:include_variables"] = options.GodotLogger.IncludeVariables;
            _receivedOptions["godot_logger:event_mask"] = (int)options.GodotLogger.EventMask;
            _receivedOptions["godot_logger:breadcrumb_mask"] = (int)options.GodotLogger.BreadcrumbMask;
            _receivedOptions["godot_logger:log_mask"] = (int)options.GodotLogger.LogMask;
            _receivedOptions["godot_logger:limits:events_per_frame"] = options.GodotLogger.Limits.EventsPerFrame;
            _receivedOptions["godot_logger:limits:repeated_error_window_ms"] = (int)options.GodotLogger.Limits.RepeatedErrorWindow.TotalMilliseconds;
            _receivedOptions["godot_logger:limits:throttle_events"] = options.GodotLogger.Limits.ThrottleEvents;
            _receivedOptions["godot_logger:limits:throttle_window_ms"] = (int)options.GodotLogger.Limits.ThrottleWindow.TotalMilliseconds;
            _receivedOptions["native:crash_reporting_mode"] = (int)options.Native.CrashReportingMode;
            _receivedOptions["native:minidump_mode"] = (int)options.Native.MinidumpMode;
            _receivedOptions["native:thread_stackwalk_mode"] = (int)options.Native.ThreadStackwalkMode;
            _receivedOptions["native:crash_upload_mode"] = (int)options.Native.CrashUploadMode;
            _receivedOptions["android:enable_anr_detection"] = options.Android.EnableAnrDetection;
            _receivedOptions["android:anr_timeout_interval_ms"] = (int)options.Android.AnrTimeoutInterval.TotalMilliseconds;
            _receivedOptions["android:attach_anr_thread_dump"] = options.Android.AttachAnrThreadDump;
            _receivedOptions["apple:enable_metrickit"] = options.Apple.EnableMetricKit;
            _receivedOptions["crash_reporter:enabled"] = options.CrashReporter.Enabled;
            _receivedOptions["crash_reporter:path"] = options.CrashReporter.Path;
            _receivedOptions["crash_reporter:window_title"] = options.CrashReporter.WindowTitle;
            _receivedOptions["crash_reporter:header_text"] = options.CrashReporter.HeaderText;
            _receivedOptions["crash_reporter:header_description"] = options.CrashReporter.HeaderDescription;
            _receivedOptions["crash_reporter:submit_button_label"] = options.CrashReporter.SubmitButtonLabel;
            _receivedOptions["crash_reporter:cancel_button_label"] = options.CrashReporter.CancelButtonLabel;
            _receivedOptions["crash_reporter:accent_color"] = options.CrashReporter.AccentColor is { } accent
                ? Color.Color8(accent.R, accent.G, accent.B, accent.A)
                : new Color(0, 0, 0, 0);
            _receivedOptions["crash_reporter:logo_light_theme"] = options.CrashReporter.LogoLightTheme;
            _receivedOptions["crash_reporter:logo_dark_theme"] = options.CrashReporter.LogoDarkTheme;
            _receivedOptions["crash_reporter:window_closable"] = options.CrashReporter.WindowClosable;
            _receivedOptions["crash_reporter:views"] = new Godot.Collections.Array<int>(options.CrashReporter.Views.Select(view => (int)view));

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
            options.TracePropagationTargets.Clear();
            options.TracePropagationTargets.Add("ccc");
            options.TracePropagationTargets.Add("ddd");
            options.PropagateTraceparent = false;
            options.OrgId = "222";
            options.AutoSessionTracking = false;
            options.AutoSessionTrackingInterval = TimeSpan.FromMilliseconds(45000);
            options.MaxBreadcrumbs = 22;
            options.ShutdownTimeout = TimeSpan.FromMilliseconds(2200);
            options.SendDefaultPii = false;
            options.AttachLog = true;
            options.AttachSceneTree = false;
            options.AttachScreenshot = false;
            options.ScreenshotLevel = SentryLevel.Warning;
            options.EnableAppHangTracking = true;
            options.AppHangTimeout = TimeSpan.FromMilliseconds(2300);
            options.AutoMetrics.EnableFrameMetrics = false;
            options.AutoMetrics.FrameMetricsIntervalSec = 2.0;
            options.AutoMetrics.EnableRenderingMetrics = false;
            options.AutoMetrics.RenderingMetricsIntervalSec = 3.0;
            options.AutoMetrics.EnableMemoryMetrics = false;
            options.AutoMetrics.MemoryMetricsIntervalSec = 45.0;
            options.AutoMetrics.EnableNetworkMetrics = false;
            options.AutoMetrics.NetworkMetricsIntervalSec = 15.0;
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
            options.Native.CrashReportingMode = SentryCrashReportingMode.Native;
            options.Native.MinidumpMode = SentryMinidumpMode.Full;
            options.Native.ThreadStackwalkMode = SentryThreadStackwalkMode.All;
            options.Native.CrashUploadMode = SentryCrashUploadMode.Sync;
            options.Android.EnableAnrDetection = true;
            options.Android.AnrTimeoutInterval = TimeSpan.FromMilliseconds(2400);
            options.Android.AttachAnrThreadDump = false;
            options.Apple.EnableMetricKit = false;
            options.CrashReporter.Enabled = false;
            options.CrashReporter.Path = "interop-managed-reporter";
            options.CrashReporter.WindowTitle = "interop-managed-title";
            options.CrashReporter.HeaderText = "interop-managed-header";
            options.CrashReporter.HeaderDescription = "interop-managed-description";
            options.CrashReporter.SubmitButtonLabel = "interop-managed-submit";
            options.CrashReporter.CancelButtonLabel = "interop-managed-cancel";
            options.CrashReporter.AccentColor = System.Drawing.Color.FromArgb(255, 0x22, 0x44, 0x66);
            options.CrashReporter.LogoLightTheme = "interop-managed-light.png";
            options.CrashReporter.LogoDarkTheme = "interop-managed-dark.png";
            options.CrashReporter.WindowClosable = true;
            options.CrashReporter.Views.Clear();
            options.CrashReporter.Views.Add(SentryCrashReporterView.Envelope);
            options.CrashReporter.Views.Add(SentryCrashReporterView.Tags);
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

    private readonly Godot.Collections.Dictionary _seenTransactionValues = [];
    public Godot.Collections.Dictionary GetSeenTransactionValues() => _seenTransactionValues;

    private int _nativeBeforeSendTransactionCallCount;
    public int GetNativeBeforeSendTransactionCallCount() => _nativeBeforeSendTransactionCallCount;

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

    private SentryNativeEvent OnBeforeSendTransaction(SentryNativeEvent transaction)
    {
        _nativeBeforeSendTransactionCallCount++;

        if (transaction.GetTag("before_send_transaction.drop") == "true")
        {
            return null;
        }

        _seenTransactionValues["release"] = transaction.Release;
        _seenTransactionValues["distribution"] = transaction.Distribution;
        _seenTransactionValues["environment"] = transaction.Environment;
        _seenTransactionValues["tag"] = transaction.GetTag("before_send_transaction.read_me");

        transaction.Release = "after-release@2.0.0";
        transaction.Distribution = "after-distribution";
        transaction.Environment = "after-environment";
        transaction.SetTag("before_send_transaction.added", "added-value");
        transaction.UnsetTag("before_send_transaction.remove_me");
        return transaction;
    }
}
