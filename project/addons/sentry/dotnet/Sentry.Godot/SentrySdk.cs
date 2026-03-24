using System;
using Godot;
using Sentry.Godot.ExceptionHandling;
using Sentry.Godot.Interop;
using Sentry.Godot.Internal;

namespace Sentry.Godot;

public class SentrySdk {
	static IDisposable _exceptionHandler;

	// TODO: Use options
	public static void Init() {
		GodotLog.Debug("Initializing Sentry in .NET...");

		Sentry.SentrySdk.Init(options => {
			options.Dsn = "https://3f1e095cf2e14598a0bd5b4ff324f712@o447951.ingest.us.sentry.io/6680910";
			options.Debug = true;
			options.SendDefaultPii = false;
			options.AddInAppExclude("Godot"); // TODO: check if it works
		});

		NativeBridge.SetTag("biome", "jungle");

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
