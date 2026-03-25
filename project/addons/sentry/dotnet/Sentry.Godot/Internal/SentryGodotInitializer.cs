using Godot;

namespace Sentry.Godot.Internal;

/// <summary>
/// Handles automatic initialization of the .NET Sentry SDK.
/// Called from [ModuleInitializer] in SentryAutoInit.cs, which is compiled into the user's assembly.
/// </summary>
public static class SentryGodotInitializer {
	public static void AutoInit() {
		var autoInit = (bool)ProjectSettings.GetSetting("sentry/options/auto_init", true);
		if (!autoInit) {
			GodotLog.Debug("Auto-init disabled in project settings. Call SentrySdk.Init() manually.");
			return;
		}
		SentrySdk.Init();
	}
}
