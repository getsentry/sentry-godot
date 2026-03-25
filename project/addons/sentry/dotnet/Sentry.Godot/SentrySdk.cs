using System;
using Godot;
using Sentry.Godot.ExceptionHandling;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot;

public class SentrySdk {
	static IDisposable? _exceptionHandler;

	public static void Init(Action<SentryGodotOptions>? configureOptions = null) {
		GodotLog.Debug("Initializing Sentry in .NET...");

		var godotOptions = new SentryGodotOptions();
		godotOptions.AddInAppExclude("Godot");
		godotOptions.ApplyNativeOptions();
		configureOptions?.Invoke(godotOptions);
		godotOptions.ApplyTemplateSubstitutions();

		Sentry.SentrySdk.Init(godotOptions);

		// TODO: On manual init, need to send signal to native layer to initialize, syncing options.
		//       On automatic init, native layer is already initialized by the time managed layer gets to act.

		InitFirstChanceExceptionHandler();
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
