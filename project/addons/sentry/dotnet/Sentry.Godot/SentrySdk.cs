using System;
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

	private static void InitDotnet(SentryGodotOptions godotOptions) {
		CurrentOptions = godotOptions;
		GodotLog.Debug("Initializing Sentry in .NET...");
		godotOptions.EnableScopeSync = true;
		godotOptions.ScopeObserver = new GodotScopeObserver();
		godotOptions.AddIntegration(new GodotSdkIntegration());
		godotOptions.AddInAppExclude("Godot");
		Sentry.SentrySdk.Init(godotOptions);
		InitFirstChanceExceptionHandler();
	}

	/// <summary>
	/// If the native SDK hasn't initialized yet (manual init case),
	/// trigger native initialization via P/Invoke.
	/// </summary>
	private static void InitNativeIfNeeded(SentryGodotOptions godotOptions) {
		if (NativeBridge.IsEnabled()) {
			return;
		}
		NativeBridge.InitNativeSdk(godotOptions);
	}

	private static void InitFirstChanceExceptionHandler() {
		// Both handlers use FirstChanceException to intercept exceptions caught by
		// Godot's bridge in try-catch blocks. They differ in how they detect the catch.
		if (NativeBridge.IsDebuggerActive()) {
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
