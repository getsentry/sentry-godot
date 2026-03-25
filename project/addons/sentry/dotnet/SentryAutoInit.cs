// This file is compiled into the user's assembly.
// It triggers automatic Sentry initialization when the assembly loads if enabled via project settings.
using System.Runtime.CompilerServices;

internal static class SentryAutoInit {
	[ModuleInitializer]
	[System.Diagnostics.CodeAnalysis.SuppressMessage("Usage", "CA2255")]
	internal static void Init() => Sentry.Godot.Internal.SentryGodotInitializer.AutoInit();
}
