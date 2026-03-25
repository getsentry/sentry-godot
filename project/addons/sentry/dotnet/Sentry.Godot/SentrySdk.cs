using System;
using Godot;
using Sentry.Godot.ExceptionHandling;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot;

public class SentrySdk {
	static IDisposable? _exceptionHandler;

	/// <summary>
	/// Initializes the .NET SDK with an optional configuration callback.
	/// </summary>
	public static void Init(Action<SentryGodotOptions>? configureOptions = null) {
		var godotOptions = new SentryGodotOptions();
		godotOptions.AddInAppExclude("Godot");
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
		godotOptions.AddInAppExclude("Godot");
		godotOptions.ApplyNativeOptions(nativeOpts);

		InitDotnet(godotOptions);
	}

	private static void InitDotnet(SentryGodotOptions godotOptions) {
		GodotLog.Debug("Initializing Sentry in .NET...");
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

		// TODO: Cache StringName instances? Should probably be selective.
		sdk.Call("init", Callable.From<GodotObject>(nativeOpts => {
			nativeOpts.Set("dsn", godotOptions.Dsn ?? "");
			nativeOpts.Set("release", godotOptions.Release ?? "");
			nativeOpts.Set("dist", godotOptions.Distribution ?? "");
			nativeOpts.Set("environment", godotOptions.Environment ?? "");
			nativeOpts.Set("debug", godotOptions.Debug);
			nativeOpts.Set("diagnostic_level", (int)godotOptions.DiagnosticLevel);
			nativeOpts.Set("sample_rate", godotOptions.SampleRate ?? 1.0);
			nativeOpts.Set("max_breadcrumbs", godotOptions.MaxBreadcrumbs);
			nativeOpts.Set("shutdown_timeout_ms", (int)godotOptions.ShutdownTimeout.TotalMilliseconds);
			nativeOpts.Set("send_default_pii", godotOptions.SendDefaultPii);
			nativeOpts.Set("attach_log", godotOptions.AttachLog);
			nativeOpts.Set("attach_scene_tree", godotOptions.AttachSceneTree);
			nativeOpts.Set("attach_screenshot", godotOptions.AttachScreenshot);
			nativeOpts.Set("screenshot_level", (int)godotOptions.ScreenshotLevel);
			nativeOpts.Set("app_hang_tracking", godotOptions.AppHangTracking);
			nativeOpts.Set("app_hang_timeout_sec", (double)godotOptions.AppHangTimeout.TotalSeconds);
			// TODO: logger_* settings
		}));
	}

	private static void InitFirstChanceExceptionHandler() {
		// FYI: We use FirstChanceException to intercept exceptions handled by the Godot
		// bridge in try-catch blocks. The exception handlers below are responsible
		// for identifying which exceptions were caught by the Godot bridge.
		if (IsCoreCLR()) {
			// CoreCLR emits ExceptionCatchStart events via EventListener.
			// This approach doesn't work with NativeAOT or MonoVM.
			GodotLog.Debug("CoreCLR runtime detected - using EventListener-based exception handler.");
			_exceptionHandler = new CoreClrExceptionHandler();
			// _exceptionHandler = new MonoExceptionHandler();
		} else {
			// MonoVM and NativeAOT don't emit ExceptionCatchStart events.
			// Fall back to Logger-based approach to detect bridge-caught exceptions.
			// Note: MonoExceptionHandler works with CoreCLR too, but it doesn't
			// intercept exceptions during debugger sessions (projects played from editor),
			// so we need both handlers.
			GodotLog.Debug("Non-CoreCLR runtime detected - using Logger-based exception handler.");
			_exceptionHandler = new MonoExceptionHandler();
		}
		GodotLog.Debug(".NET first chance exception handler initialized.");
	}

	private static bool IsCoreCLR() {
		if (!System.Runtime.CompilerServices.RuntimeFeature.IsDynamicCodeSupported) {
			// No dynamic code support -> NativeAOT
			return false;
		}

		var hasMonoRuntime = Type.GetType("Mono.RuntimeStructs, System.Private.CoreLib") != null;
		if (hasMonoRuntime) {
			return false;
		}

		return true;
	}
}
