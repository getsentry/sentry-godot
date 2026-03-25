using Godot;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot;

public sealed class SentryGodotOptions : SentryOptions {
	// NOTE: Custom options are defined in GDExtension.
	//       Here we mirror those that don't exist in .NET SDK.

	private enum DebugPrinting {
		Off = 0,
		On = 1,
		Auto = 2,
	}

	public bool AttachLog { get; set; } = true;

	public bool AttachSceneTree { get; set; } = false;

	public bool AttachScreenshot { get; set; } = false;
	public SentryLevel ScreenshotLevel { get; set; } = SentryLevel.Fatal;

	internal void ApplyProjectSettings() {
		var dsn = (string)ProjectSettings.GetSetting("sentry/options/dsn");
		if (!string.IsNullOrEmpty(dsn)) {
			Dsn = dsn;
		}

		var release = (string)ProjectSettings.GetSetting("sentry/options/release");
		if (!string.IsNullOrEmpty(release)) {
			Release = release;
		}

		var dist = (string)ProjectSettings.GetSetting("sentry/options/dist");
		if (!string.IsNullOrEmpty(dist)) {
			Distribution = dist;
		}

		var environment = (string)ProjectSettings.GetSetting("sentry/options/environment");
		if (!string.IsNullOrEmpty(environment)) {
			Environment = environment;
		}

		var debugMode = (DebugPrinting)(int)ProjectSettings.GetSetting("sentry/options/debug_printing");
		Debug = debugMode == DebugPrinting.On || (debugMode == DebugPrinting.Auto && OS.IsDebugBuild());
		DiagnosticLevel = (SentryLevel)(int)ProjectSettings.GetSetting("sentry/options/diagnostic_level", (int)SentryLevel.Debug);

		SampleRate = (float)ProjectSettings.GetSetting("sentry/options/sample_rate", 1.0);
		MaxBreadcrumbs = (int)ProjectSettings.GetSetting("sentry/options/max_breadcrumbs", 100);
		ShutdownTimeout = System.TimeSpan.FromMilliseconds(
				(int)ProjectSettings.GetSetting("sentry/options/shutdown_timeout_ms", 2000));
		SendDefaultPii = (bool)ProjectSettings.GetSetting("sentry/options/send_default_pii", false);

		AttachLog = (bool)ProjectSettings.GetSetting("sentry/options/attach_log", AttachLog);
		AttachSceneTree = (bool)ProjectSettings.GetSetting("sentry/options/attach_scene_tree", AttachSceneTree);

		AttachScreenshot = (bool)ProjectSettings.GetSetting("sentry/experimental/attach_screenshot", AttachScreenshot);
		ScreenshotLevel = (SentryLevel)(int)ProjectSettings.GetSetting("sentry/experimental/screenshot_level", (int)SentryLevel.Fatal);

		EnableLogs = (bool)ProjectSettings.GetSetting("sentry/options/enable_logs", false);
		Experimental.EnableMetrics = (bool)ProjectSettings.GetSetting("sentry/experimental/enable_metrics", true);

		// TODO: app_hang_tracking
		// TODO: app_hang_timeout_sec

		// TODO: logger_enabled
		// TODO: logger_inlude_source
		// TODO: logger_include_variables
		// TODO: logger_messages_as_breadcrumbs
		// TODO: logger_event_mask
		// TODO: logger_breadcrumb_mask

		// TODO: logger_limits

		// TODO: Options need to sync with GDExtension (aka Native).
	}

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
