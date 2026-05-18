using System;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot;

public sealed class SentryGodotOptions : SentryOptions
{
    public SentryGodotOptions()
    {
        IsGlobalModeEnabled = true;
        EnableScopeSync = true;
        ScopeObserver = new GodotScopeObserver();

        // Native layer owns sessions; .NET-side tracking would double-count.
        AutoSessionTracking = false;

        AddInAppExclude("Godot");
        AddIntegration(new GodotSdkIntegration());

        AddEventProcessor(new DefaultAttachmentsProcessor());
    }

    /// <summary>
    /// If enabled, the SDK will attach the Godot log file to the event.
    /// </summary>
    public bool AttachLog { get; set; } = true;

    /// <summary>
    /// If enabled, the SDK captures scene tree hierarchy data with each event.
    /// </summary>
    public bool AttachSceneTree { get; set; } = false;

    /// <summary>
    /// If enabled, the SDK captures screenshots for events meeting or exceeding <see cref="ScreenshotLevel"/>.
    /// </summary>
    /// <remarks>
    /// This feature is experimental and may impact performance. Test before enabling in production.
    /// </remarks>
    public bool AttachScreenshot { get; set; } = false;

    /// <summary>
    /// Minimum event level for which screenshots are captured.
    /// </summary>
    public SentryLevel ScreenshotLevel { get; set; } = SentryLevel.Fatal;

    /// <summary>
    /// If enabled, the SDK monitors the main thread and reports hang events
    /// when it becomes unresponsive for longer than <see cref="AppHangTimeout"/>.
    /// </summary>
    /// <remarks>
    /// Only supported on Android, iOS, and macOS.
    /// </remarks>
    public bool AppHangTracking { get; set; } = false;

    /// <summary>
    /// Duration after which the application is considered to have hanged.
    /// </summary>
    public TimeSpan AppHangTimeout { get; set; } = TimeSpan.FromSeconds(5.0);

    /// <summary>
    /// If enabled, the SDK captures logged GDScript and engine runtime errors as events and/or breadcrumbs,
    /// as defined by <see cref="LoggerEventMask"/> and <see cref="LoggerBreadcrumbMask"/>.
    /// </summary>
    /// <remarks>
    /// This setting controls GDScript/engine error reporting, not .NET errors.
    /// </remarks>
    public bool LoggerEnabled { get; set; } = true;

    /// <summary>
    /// If enabled, the SDK includes surrounding source code of logged errors, if available.
    /// </summary>
    /// <remarks>
    /// This setting controls GDScript error reporting, not .NET errors.
    /// </remarks>
    public bool LoggerIncludeSource { get; set; } = false;

    /// <summary>
    /// If enabled, the SDK includes local variables from stack traces when capturing script errors.
    /// </summary>
    /// <remarks>
    /// May impact performance, especially for applications with frequent errors or deep call stacks.
    /// This setting controls GDScript error reporting, not .NET errors.
    /// </remarks>
    public bool LoggerIncludeVariables { get; set; } = false;

    /// <summary>
    /// Bitfield mask for Godot logger events.
    /// </summary>
    /// <remarks>
    /// Used to specify which Godot logger events are captured as Sentry events, breadcrumbs and logs.
    /// </remarks>
    [Flags]
    public enum GodotLoggerEventMask
    {
        /// <summary>No logger errors or messages will be captured.</summary>
        None = 0,
        /// <summary>Native (C++) and engine errors, which may also originate from a script.</summary>
        Error = 1 << 0,
        /// <summary>Warnings.</summary>
        Warning = 1 << 1,
        /// <summary>Script errors.</summary>
        Script = 1 << 2,
        /// <summary>Shader errors.</summary>
        Shader = 1 << 3,
        // NOTE: Bits 4-6 are reserved for any additional Godot error types that may be added in the future.
        /// <summary>Log messages such as <c>GD.Print()</c> statements.</summary>
        Message = 1 << 7
    }

    /// <summary>
    /// Specifies which Godot logger events are captured as Sentry events.
    /// </summary>
    /// <remarks>
    /// <see cref="GodotLoggerEventMask.Message"/> has no effect here. To capture log messages,
    /// set it in <see cref="LoggerLogMask"/> or <see cref="LoggerBreadcrumbMask"/> instead.
    /// </remarks>
    public GodotLoggerEventMask LoggerEventMask { get; set; } = GodotLoggerEventMask.Error | GodotLoggerEventMask.Script | GodotLoggerEventMask.Shader;

    /// <summary>
    /// Specifies which Godot logger events are captured as Sentry breadcrumbs.
    /// </summary>
    public GodotLoggerEventMask LoggerBreadcrumbMask { get; set; } = GodotLoggerEventMask.Error | GodotLoggerEventMask.Warning | GodotLoggerEventMask.Script | GodotLoggerEventMask.Shader | GodotLoggerEventMask.Message;

    /// <summary>
    /// Specifies which Godot logger events are captured as Sentry logs.
    /// </summary>
    public GodotLoggerEventMask LoggerLogMask { get; set; } = GodotLoggerEventMask.None;

    /// <summary>
    /// Defines throttling limits for the error logger. These limits are used to prevent the SDK from sending
    /// too many non-critical and repeating error events. See <see cref="SentryLoggerLimits"/>.
    /// </summary>
    /// <remarks>
    /// This setting controls GDScript/engine error reporting, not .NET errors.
    /// </remarks>
    public SentryLoggerLimits LoggerLimits { get; set; } = new SentryLoggerLimits();

    /// <summary>
    /// Reads resolved options from the native SentryOptions.
    /// </summary>
    /// <remarks>
    /// Since the native SDK initializes before .NET, treat native options as single
    /// source of truth for those options that are common to both.
    /// </remarks>
    internal void ApplyNativeOptions()
    {
        NativeBridge.ApplyNativeOptions(this);
    }

    /// <summary>
    /// Applies default option values from the native layer, which may be overridden by the user callback.
    /// </summary>
    internal void ApplyNativeOptionsDefaults()
    {
        NativeBridge.ApplyNativeOptionsDefaults(this);
    }

    /// <summary>
    /// Applies template substitutions to options that may have been set by user callback.
    /// </summary>
    internal void ApplyTemplateSubstitutions()
    {
        if (Release != null)
        {
            Release = Release.Replace("{app_name}", NativeBridge.GetAppName())
                              .Replace("{app_version}", NativeBridge.GetAppVersion());
        }

        if (Environment == "{auto}")
        {
            var detectedEnv = NativeBridge.DetectEnvironment();
            if (detectedEnv != null)
            {
                Environment = detectedEnv;
            }
            else
            {
                GodotLog.Error("Failed to detect environment automatically.");
            }
        }
    }
}

/// <summary>
/// Specifies throttling limits for the error logger.
/// These limits govern the behavior of throttling and are used to prevent the SDK from sending
/// too many non-critical and repeating error events.
/// </summary>
public sealed class SentryLoggerLimits
{
    /// <summary>
    /// Specifies the maximum number of error events to send per processed frame. If exceeded,
    /// no further errors will be captured until the next frame.
    /// </summary>
    public int EventsPerFrame { get; set; } = 5;

    /// <summary>
    /// Specifies the minimum time interval between two identical errors. If exceeded,
    /// no further errors from the same line of code with the identical message will be
    /// captured until the next interval. Set to zero to disable this limit.
    /// </summary>
    public TimeSpan RepeatedErrorWindow { get; set; } = TimeSpan.FromMilliseconds(1000);

    /// <summary>
    /// Specifies the maximum number of events allowed within a sliding time window
    /// of <see cref="ThrottleWindow"/>. If exceeded, errors will be captured as
    /// breadcrumbs only until capacity is freed.
    /// </summary>
    public int ThrottleEvents { get; set; } = 20;

    /// <summary>
    /// Specifies the time window for <see cref="ThrottleEvents"/>.
    /// Set to zero to disable this limit.
    /// </summary>
    public TimeSpan ThrottleWindow { get; set; } = TimeSpan.FromMilliseconds(10000);
}
