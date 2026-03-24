using Godot;

namespace Sentry.Godot.Internal;

/// <summary>
/// Wrapper around Godot's logging functions.
/// </summary>
internal static class GodotLog {
	internal static void Debug(string message) {
		GD.Print($"Sentry: DEBUG: {message}");
	}

	internal static void Info(string message) {
		GD.Print($"Sentry: INFO: {message}");
	}

	internal static void Warn(string message) {
		GD.PrintErr($"Sentry: WARN: {message}");
	}

	internal static void Error(string message) {
		GD.PrintErr($"Sentry: ERROR: {message}");
	}
}
