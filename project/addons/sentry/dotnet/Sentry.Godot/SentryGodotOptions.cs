using Godot;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot;

public sealed class SentryGodotOptions : SentryOptions {
	private enum DebugPrinting {
		Off = 0,
		On = 1,
		Auto = 2,
	}

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

		SampleRate = (float)ProjectSettings.GetSetting("sentry/options/sample_rate", 1.0);
		MaxBreadcrumbs = (int)ProjectSettings.GetSetting("sentry/options/max_breadcrumbs", 100);
		SendDefaultPii = (bool)ProjectSettings.GetSetting("sentry/options/send_default_pii", false);
		DiagnosticLevel = (SentryLevel)(int)ProjectSettings.GetSetting("sentry/options/diagnostic_level", (int)SentryLevel.Debug);

		// TODO: shutdown_timeout_ms
		// TODO: enable_logs
		// TODO: enable_metrics
		// TODO: attach_log, attach_scene_tree
		// TODO: app_hang?
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
