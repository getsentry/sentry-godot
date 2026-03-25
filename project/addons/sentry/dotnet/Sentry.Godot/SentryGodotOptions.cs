using System;
using Godot;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot;

public sealed class SentryGodotOptions : SentryOptions {
	public bool AttachLog { get; set; } = true;

	public bool AttachSceneTree { get; set; } = false;

	public bool AttachScreenshot { get; set; } = false;
	public SentryLevel ScreenshotLevel { get; set; } = SentryLevel.Fatal;

	public bool AppHangTracking { get; set; } = false;
	public System.TimeSpan AppHangTimeout { get; set; } = System.TimeSpan.FromSeconds(5.0);

	/// <summary>
	/// Enables logger that reports GDScript and engine runtime errors to Sentry.
	/// </summary>
	/// <remarks>
	/// This setting does not control .NET errors.
	/// </remarks>
	public bool LoggerEnabled { get; set; } = true;

	public bool LoggerIncludeSource { get; set; } = false;
	public bool LoggerIncludeVariables { get; set; } = false;
	public bool LoggerMessagesAsBreadcrumbs { get; set; } = true;

	[Flags]
	public enum GodotErrorMask {
		None = 0,
		Error = 1,
		Warning = 2,
		Script = 4,
		Shader = 8,
	}

	public GodotErrorMask LoggerEventMask = GodotErrorMask.Error | GodotErrorMask.Script | GodotErrorMask.Shader;

	public GodotErrorMask LoggerBreadcrumbMask = GodotErrorMask.Error | GodotErrorMask.Script | GodotErrorMask.Shader | GodotErrorMask.Warning;

	// TODO: add logger limits

	/// <summary>
	/// Reads resolved options from the native SentryOptions.
	/// </summary>
	/// <remarks>
	/// Since the native SDK initializes before .NET, treat native options as single
	/// source of truth for those options that are common to both.
	/// </remarks>
	internal void ApplyNativeOptions() {
		if (!ClassDB.ClassExists("SentryOptions")) {
			GodotLog.Error("SentryOptions class not found! Is the Sentry GDExtension loaded?");
			return;
		}

		var nativeOpts = ClassDB.ClassCallStatic("SentryOptions", "create_from_project_settings")
								 .AsGodotObject();
		if (nativeOpts == null) {
			GodotLog.Error("Internal: Failed to create native SentryOptions from project settings.");
			return;
		}

		ApplyNativeOptions(nativeOpts);
	}

	/// <summary>
	/// Reads resolved options from the provided SentryOptions object from native layer.
	/// </summary>
	internal void ApplyNativeOptions(GodotObject nativeOpts) {
		Dsn = (string)nativeOpts.Get("dsn");
		Release = (string)nativeOpts.Get("release");
		Distribution = (string)nativeOpts.Get("dist");
		Environment = (string)nativeOpts.Get("environment");
		Debug = (bool)nativeOpts.Get("debug");
		DiagnosticLevel = (SentryLevel)(int)nativeOpts.Get("diagnostic_level");
		SampleRate = (float)nativeOpts.Get("sample_rate");
		MaxBreadcrumbs = (int)nativeOpts.Get("max_breadcrumbs");
		ShutdownTimeout = System.TimeSpan.FromMilliseconds((int)nativeOpts.Get("shutdown_timeout_ms"));
		SendDefaultPii = (bool)nativeOpts.Get("send_default_pii");
		EnableLogs = (bool)nativeOpts.Get("enable_logs");

		// Godot-specific options
		AttachLog = (bool)nativeOpts.Get("attach_log");
		AttachSceneTree = (bool)nativeOpts.Get("attach_scene_tree");
		AttachScreenshot = (bool)nativeOpts.Get("attach_screenshot");
		ScreenshotLevel = (SentryLevel)(int)nativeOpts.Get("screenshot_level");
		AppHangTracking = (bool)nativeOpts.Get("app_hang_tracking");
		AppHangTimeout = System.TimeSpan.FromSeconds((double)nativeOpts.Get("app_hang_timeout_sec"));

		// Logger options
		LoggerEnabled = (bool)nativeOpts.Get("logger_enabled");
		LoggerIncludeSource = (bool)nativeOpts.Get("logger_include_source");
		LoggerIncludeVariables = (bool)nativeOpts.Get("logger_include_variables");
		LoggerMessagesAsBreadcrumbs = (bool)nativeOpts.Get("logger_messages_as_breadcrumbs");
		LoggerEventMask = (GodotErrorMask)(int)nativeOpts.Get("logger_event_mask");
		LoggerBreadcrumbMask = (GodotErrorMask)(int)nativeOpts.Get("logger_breadcrumb_mask");
	}

	/// <summary>
	/// Syncs the options to the provided SentryOptions object from native layer.
	/// </summary>
	internal void SyncToNativeOptions(GodotObject nativeOpts) {
		nativeOpts.Set("dsn", Dsn ?? "");
		nativeOpts.Set("release", Release ?? "");
		nativeOpts.Set("dist", Distribution ?? "");
		nativeOpts.Set("environment", Environment ?? "");
		nativeOpts.Set("debug", Debug);
		nativeOpts.Set("diagnostic_level", (int)DiagnosticLevel);
		nativeOpts.Set("sample_rate", SampleRate ?? 1.0);
		nativeOpts.Set("max_breadcrumbs", MaxBreadcrumbs);
		nativeOpts.Set("shutdown_timeout_ms", (int)ShutdownTimeout.TotalMilliseconds);
		nativeOpts.Set("send_default_pii", SendDefaultPii);

		nativeOpts.Set("attach_log", AttachLog);
		nativeOpts.Set("attach_scene_tree", AttachSceneTree);
		nativeOpts.Set("attach_screenshot", AttachScreenshot);
		nativeOpts.Set("screenshot_level", (int)ScreenshotLevel);
		nativeOpts.Set("app_hang_tracking", AppHangTracking);
		nativeOpts.Set("app_hang_timeout_sec", (double)AppHangTimeout.TotalSeconds);

		nativeOpts.Set("logger_enabled", LoggerEnabled);
		nativeOpts.Set("logger_include_source", LoggerIncludeSource);
		nativeOpts.Set("logger_include_variables", LoggerIncludeVariables);
		nativeOpts.Set("logger_messages_as_breadcrumbs", LoggerMessagesAsBreadcrumbs);
		nativeOpts.Set("logger_event_mask", (int)LoggerEventMask);
		nativeOpts.Set("logger_breadcrumb_mask", (int)LoggerBreadcrumbMask);
	}

	/// <summary>
	/// Applies template substitutions to options that may have been set by user callback.
	/// </summary>
	internal void ApplyTemplateSubstitutions() {
		if (Release != null) {
			Release = Release.Replace("{app_name}", (string)ProjectSettings.GetSetting("application/config/name"))
							  .Replace("{app_version}", (string)ProjectSettings.GetSetting("application/config/version"));
		}

		if (Environment == "{auto}") {
			// Auto-detection is defined in the native layer.
			var detectedEnv = NativeBridge.DetectEnvironment();
			if (detectedEnv != null) {
				Environment = detectedEnv;
			} else {
				GodotLog.Error("Internal: Failed to detect environment automatically.");
			}
		}
	}
}
