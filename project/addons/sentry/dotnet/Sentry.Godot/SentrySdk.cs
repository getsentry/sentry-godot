using System;
using Godot;
using Sentry.Godot.ExceptionHandling;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot;

public class SentrySdk {
	static IDisposable? _exceptionHandler;
	internal static SentryGodotOptions? CurrentOptions { get; private set; }

	/// <summary>
	/// Initializes the .NET SDK with an optional configuration callback.
	/// </summary>
	public static void Init(Action<SentryGodotOptions>? configureOptions = null) {
		var godotOptions = new SentryGodotOptions();
		godotOptions.ApplyNativeOptions();
		configureOptions?.Invoke(godotOptions);
		godotOptions.ApplyTemplateSubstitutions();

		InitDotnet(godotOptions);
		InitNativeIfNeeded(godotOptions);
	}

	/// <summary>
	/// Initializes the .NET SDK from a native SentryOptions object.
	/// Called by the native init callback when native layer initializes first.
	/// </summary>
	internal static void InitFromNativeOptions(GodotObject nativeOpts) {
		var godotOptions = new SentryGodotOptions();
		godotOptions.ApplyNativeOptions(nativeOpts);

		InitDotnet(godotOptions);
	}

	private static void InitDotnet(SentryGodotOptions godotOptions) {
		CurrentOptions = godotOptions;
		GodotLog.Debug("Initializing Sentry in .NET...");
		godotOptions.EnableScopeSync = true;
		godotOptions.ScopeObserver = new GodotScopeObserver();
		godotOptions.AddIntegration(new GodotSdkIntegration());
		godotOptions.AddInAppExclude("Godot"); // Godot's bridge layer (aka mono glue)
		Sentry.SentrySdk.Init(godotOptions);
		InitFirstChanceExceptionHandler();
	}

	/// <summary>
	/// If the native SDK hasn't initialized yet (in case of a manual init),
	/// signal it to init with options synced from the .NET configuration callback.
	/// </summary>
	private static void InitNativeIfNeeded(SentryGodotOptions godotOptions) {
		var sdk = Engine.GetSingleton("SentrySDK");
		if (sdk == null || (bool)sdk.Call("is_enabled")) {
			return;
		}

		sdk.Call("init", Callable.From<GodotObject>(godotOptions.SyncToNativeOptions));
	}

	private static void InitFirstChanceExceptionHandler() {
		// Both handlers use FirstChanceException to intercept exceptions caught by
		// Godot's bridge in try-catch blocks. They differ in how they detect the catch.
		if (EngineDebugger.IsActive()) {
			// During debugger sessions (projects played from editor), Godot suppresses
			// error logging, so LoggerExceptionHandler can't detect bridge catches.
			// CoreClrExceptionHandler uses EventListener events instead - works with
			// debugger but has more overhead and only supports CoreCLR (i.e. PC and Mac).
			GodotLog.Debug("Debugger active - using EventListener-based exception handler.");
			_exceptionHandler = new CoreClrExceptionHandler();
		} else {
			// LoggerExceptionHandler uses Godot's Logger to detect bridge catches.
			// More efficient and works on all runtimes (CoreCLR, MonoVM, NativeAOT).
			GodotLog.Debug("Using Logger-based exception handler.");
			_exceptionHandler = new LoggerExceptionHandler();
		}
		GodotLog.Debug(".NET first chance exception handler initialized.");
	}
}
