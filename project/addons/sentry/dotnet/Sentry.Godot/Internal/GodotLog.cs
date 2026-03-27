using Godot;

namespace Sentry.Godot.Internal;

/// <summary>
/// Wrapper around Godot's logging functions.
/// </summary>
internal static class GodotLog {
	private static bool ShouldPrint(SentryLevel level) {
		var opts = SentrySdk.CurrentOptions;
		return opts is null || (opts.Debug && opts.DiagnosticLevel >= level);
	}

	internal static void Debug(string message) {
		if (ShouldPrint(SentryLevel.Debug)) {
			GD.Print("Sentry: DEBUG: " + message);
		}
	}

	internal static void Info(string message) {
		if (ShouldPrint(SentryLevel.Info)) {
			GD.Print("Sentry: INFO: " + message);
		}
	}

	internal static void Warn(string message) {
		if (ShouldPrint(SentryLevel.Warning)) {
			GD.PrintErr("Sentry: WARN: " + message);
		}
	}

	internal static void Error(string message) {
		if (ShouldPrint(SentryLevel.Error)) {
			GD.PrintErr("Sentry: ERROR: " + message);
		}
	}
}
