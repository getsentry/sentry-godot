using System;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;
using System.Collections.Generic;

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
    /// This feature applies to iOS and macOS only. On Android, <see cref="Android"/> configures
    /// ANR (Application Not Responding) detection instead.
    /// </remarks>
    public bool EnableAppHangTracking { get; set; } = false;

    /// <summary>
    /// Duration after which the application is considered to have hanged.
    /// </summary>
    public TimeSpan AppHangTimeout { get; set; } = TimeSpan.FromSeconds(5.0);

    /// <summary>
    /// Configures the capture of Godot errors and log messages as Sentry events, breadcrumbs,
    /// and logs. See <see cref="SentryGodotLoggerOptions"/>.
    /// </summary>
    public SentryGodotLoggerOptions GodotLogger { get; set; } = new SentryGodotLoggerOptions();

    /// <summary>
    /// Configures Android-specific options, such as ANR (Application Not Responding) detection.
    /// </summary>
    public SentryAndroidOptions Android { get; set; } = new SentryAndroidOptions();

    private readonly List<SentryAttachment> _defaultAttachments = [];

    internal IReadOnlyList<SentryAttachment> DefaultAttachments => _defaultAttachments;

    internal void AddDefaultAttachment(SentryAttachment attachment)
    {
        if (attachment is null)
        {
            return;
        }
        _defaultAttachments.Add(attachment);
    }

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
/// Contains configuration options that control how the SDK captures Godot errors and log messages
/// as Sentry events, breadcrumbs, and logs. Access this configuration through
/// <see cref="SentryGodotOptions.GodotLogger"/>.
/// </summary>
/// <remarks>
/// This includes engine errors, GDScript and shader errors, GDExtension errors, <c>GD.PushError()</c>
/// and <c>GD.PushWarning()</c> statements, as well as log messages from <c>GD.Print()</c> and
/// similar functions.
/// These options control Godot-side error reporting, not .NET errors.
/// </remarks>
/// <seealso cref="SentryGodotOptions"/>
public sealed class SentryGodotLoggerOptions
{
    /// <summary>
    /// If enabled, the SDK will capture logged errors as events, breadcrumbs and/or logs, as defined
    /// by <see cref="EventMask"/>, <see cref="BreadcrumbMask"/> and <see cref="LogMask"/>.
    /// Crashes are always captured. See also <see cref="SentryOptions.EnableLogs"/>.
    /// </summary>
    /// <remarks>
    /// This setting controls GDScript/engine error reporting, not .NET errors.
    /// </remarks>
    public bool Enabled { get; set; } = true;

    /// <summary>
    /// If enabled, the SDK will include the surrounding source code of logged errors, if available
    /// in the exported project.
    /// </summary>
    /// <remarks>
    /// This setting controls GDScript error reporting, not .NET errors.
    /// </remarks>
    public bool IncludeSourceContext { get; set; } = false;

    /// <summary>
    /// If enabled, the SDK will include local variables from stack traces when capturing script errors.
    /// This allows showing the values of variables at each frame in the call stack. Requires enabling
    /// the <c>debug/settings/gdscript/always_track_local_variables</c> project setting.
    /// </summary>
    /// <remarks>
    /// May impact performance, especially for applications with frequent errors or deep call stacks.
    /// This setting controls GDScript error reporting, not .NET errors.
    /// </remarks>
    public bool IncludeVariables { get; set; } = false;

    /// <summary>
    /// Specifies the Godot logger events that are automatically captured as Sentry events.
    /// Accepts a single value or a bitwise combination of <see cref="GodotLoggerEventMask"/> masks.
    /// </summary>
    /// <remarks>
    /// <see cref="GodotLoggerEventMask.Message"/> has no effect here. To capture log messages,
    /// set it in <see cref="LogMask"/> and/or <see cref="BreadcrumbMask"/> instead.
    /// </remarks>
    public GodotLoggerEventMask EventMask { get; set; } = GodotLoggerEventMask.Error | GodotLoggerEventMask.Script | GodotLoggerEventMask.Shader;

    /// <summary>
    /// Specifies the Godot logger events that are automatically captured as Sentry breadcrumbs.
    /// Accepts a single value or a bitwise combination of <see cref="GodotLoggerEventMask"/> masks.
    /// </summary>
    public GodotLoggerEventMask BreadcrumbMask { get; set; } = GodotLoggerEventMask.Error | GodotLoggerEventMask.Warning | GodotLoggerEventMask.Script | GodotLoggerEventMask.Shader | GodotLoggerEventMask.Message;

    /// <summary>
    /// Specifies the Godot logger events that are automatically captured as Sentry logs.
    /// Accepts a single value or a bitwise combination of <see cref="GodotLoggerEventMask"/> masks.
    /// Empty by default, so no events are captured as logs.
    /// </summary>
    /// <remarks>
    /// Log capture requires <see cref="SentryOptions.EnableLogs"/> to be enabled.
    /// </remarks>
    public GodotLoggerEventMask LogMask { get; set; } = GodotLoggerEventMask.None;

    /// <summary>
    /// Defines throttling limits for the error logger. These limits are used to prevent the SDK
    /// from sending too many non-critical and repeating error events. See <see cref="SentryLoggerLimits"/>.
    /// </summary>
    /// <remarks>
    /// This setting controls GDScript/engine error reporting, not .NET errors.
    /// </remarks>
    public SentryLoggerLimits Limits { get; set; } = new SentryLoggerLimits();
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

/// <summary>
/// Contains configuration options that apply only when the project runs on Android, such as ANR (Application Not
/// Responding) detection. Access this configuration through <see cref="SentryGodotOptions.Android"/>.
/// </summary>
/// <seealso cref="SentryGodotOptions"/>
public sealed class SentryAndroidOptions
{
    /// <summary>
    /// Enables detection and reporting of ANR (Application Not Responding) errors. The SDK monitors the main thread
    /// for unresponsiveness and reports an event when an ANR occurs.
    /// </summary>
    /// <remarks>
    /// Android 11 and later use the system-based V2 implementation, while earlier versions use the watchdog-based V1
    /// implementation. See <see cref="AnrTimeoutInterval"/> and <see cref="AttachAnrThreadDump"/> for options specific
    /// to each implementation. On Apple platforms, <see cref="SentryGodotOptions.EnableAppHangTracking"/> configures the
    /// equivalent app hang detection.
    /// To learn more, visit <see href="https://docs.sentry.io/platforms/android/configuration/app-not-respond/">Application Not Responding documentation</see>.
    /// </remarks>
    public bool EnableAnrDetection { get; set; } = true;

    /// <summary>
    /// Specifies how long the main thread must stay blocked before the SDK reports an ANR.
    /// </summary>
    /// <remarks>
    /// Applies only when <see cref="EnableAnrDetection"/> is enabled, and only to the V1 implementation used on Android
    /// versions before 11. On Android 11 and later, the operating system determines when the application stops responding, so
    /// this value has no effect.
    /// </remarks>
    public TimeSpan AnrTimeoutInterval { get; set; } = TimeSpan.FromMilliseconds(5000);

    /// <summary>
    /// Attaches the operating system's thread dump to the ANR event as a plain-text attachment, adding detail for
    /// investigating where the application became unresponsive.
    /// </summary>
    /// <remarks>
    /// Applies only when <see cref="EnableAnrDetection"/> is enabled, and only to the V2 implementation used on Android
    /// 11 and later.
    /// </remarks>
    public bool AttachAnrThreadDump { get; set; } = false;
}
