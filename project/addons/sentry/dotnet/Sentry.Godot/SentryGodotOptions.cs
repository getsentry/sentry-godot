using Godot;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot;

public sealed class SentryGodotOptions : SentryOptions {
	public bool AttachLog { get; set; } = true;

	public bool AttachSceneTree { get; set; } = false;

	public bool AttachScreenshot { get; set; } = false;
	public SentryLevel ScreenshotLevel { get; set; } = SentryLevel.Fatal;

	/// <summary>
	/// Reads resolved options from the native GDExtension SentryOptions.
	/// </summary>
	/// <remarks>
	/// Since the native SDK initializes before .NET, treat it as single
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
			GodotLog.Error("Failed to create native SentryOptions from project settings.");
			return;
		}

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

		// TODO: app_hang_tracking, app_hang_timeout_sec
		// TODO: logger_* settings
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
			// Native layer defines auto-detection
			var detectedEnv = NativeBridge.DetectEnvironment();
			if (detectedEnv != null) {
				Environment = detectedEnv;
			} else {
				GodotLog.Error("Internal: Failed to detect environment automatically.");
			}
		}
	}
}
