using Sentry.Godot.Interop;

namespace Sentry.Godot.Internal;

/// <summary>
/// Wrapper around native logging functions via P/Invoke.
/// </summary>
internal static class GodotLog {
	private static bool ShouldPrint(SentryLevel level) {
		var opts = SentrySdk.CurrentOptions;
		return opts is null || (opts.Debug && level >= opts.DiagnosticLevel);
	}

	internal static void Debug(string message) {
		if (ShouldPrint(SentryLevel.Debug)) {
			NativeBridge.Log(SentryLevel.Debug, message);
		}
	}

	internal static void Info(string message) {
		if (ShouldPrint(SentryLevel.Info)) {
			NativeBridge.Log(SentryLevel.Info, message);
		}
	}

	internal static void Warn(string message) {
		if (ShouldPrint(SentryLevel.Warning)) {
			NativeBridge.Log(SentryLevel.Warning, message);
		}
	}

	internal static void Error(string message) {
		if (ShouldPrint(SentryLevel.Error)) {
			NativeBridge.Log(SentryLevel.Error, message);
		}
	}
}
