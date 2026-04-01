using Sentry.Godot.Interop;

namespace Sentry.Godot.Internal;

/// <summary>
/// Handles automatic initialization of the .NET Sentry SDK.
/// Called from [ModuleInitializer] in SentryAutoInit.cs, which is compiled into the user's assembly.
/// </summary>
public static class SentryGodotInitializer {
	public static void AutoInit() {
		NativeBridge.RegisterDotnetInit();
		if (NativeBridge.IsEnabled()) {
			GodotLog.Debug("Native is already initialized, proceeding with .NET initialization.");
			SentrySdk.Init();
		}
	}
}
