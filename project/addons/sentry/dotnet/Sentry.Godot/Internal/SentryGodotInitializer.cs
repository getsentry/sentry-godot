using Godot;

namespace Sentry.Godot.Internal;

/// <summary>
/// Handles automatic initialization of the .NET Sentry SDK.
/// Called from [ModuleInitializer] in SentryAutoInit.cs, which is compiled into the user's assembly.
/// </summary>
public static class SentryGodotInitializer {
	public static void AutoInit() {
		var sdk = Engine.GetSingleton("SentrySDK");
		if (sdk == null) {
			GodotLog.Error("SentrySDK singleton not found. Is the Sentry GDExtension loaded?");
			return;
		}

		if ((bool)sdk.Call("is_enabled")) {
			// Native layer already initialized (in case of automatic initialization).
			// So initialize the managed layer syncing native options.
			SentrySdk.Init();
		} else {
			// Native layer hasn't initialized yet - register callback so .NET can init
			// when native init() is called (e.g. from GDScript).
			sdk.Call("_set_dotnet_init_callback",
					Callable.From<GodotObject>(OnNativeInit));
			GodotLog.Debug("Registered .NET init callback with native SDK.");
		}
	}

	private static void OnNativeInit(GodotObject nativeOpts) {
		GodotLog.Debug("Native layer initialized - proceeding to initialize .NET.");
		SentrySdk.InitFromNativeOptions(nativeOpts);
	}
}
