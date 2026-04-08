using System;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot;

public sealed class SentryGodotOptions : SentryOptions
{
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
    /// If enabled, log messages (such as <c>Print()</c> statements) are captured as breadcrumbs.
    /// </summary>
    public bool LoggerMessagesAsBreadcrumbs { get; set; } = true;

    /// <summary>
    /// Bitfield mask for Godot error types.
    /// </summary>
    [Flags]
    public enum GodotErrorMask
    {
        /// <summary>No errors captured.</summary>
        None = 0,
        /// <summary>Native (C++) errors, which may also originate from a script.</summary>
        Error = 1,
        /// <summary>Warnings.</summary>
        Warning = 2,
        /// <summary>Script errors.</summary>
        Script = 4,
        /// <summary>Shader errors.</summary>
        Shader = 8,
    }

    /// <summary>
    /// Specifies which error types are captured as Sentry events.
    /// </summary>
    public GodotErrorMask LoggerEventMask { get; set; } = GodotErrorMask.Error | GodotErrorMask.Script | GodotErrorMask.Shader;

    /// <summary>
    /// Specifies which error types are captured as breadcrumbs.
    /// </summary>
    public GodotErrorMask LoggerBreadcrumbMask { get; set; } = GodotErrorMask.Error | GodotErrorMask.Script | GodotErrorMask.Shader | GodotErrorMask.Warning;

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
