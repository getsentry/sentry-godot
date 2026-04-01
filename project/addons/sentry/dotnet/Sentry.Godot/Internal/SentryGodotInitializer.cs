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
			// Native layer is already initialized - initialize .NET layer.
			SentrySdk.Init();
		}
	}
}
