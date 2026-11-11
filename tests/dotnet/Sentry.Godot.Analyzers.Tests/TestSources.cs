namespace Sentry.Godot.Analyzers.Tests;

internal static class TestSources
{
    // Minimal stand-in for the real Sentry.Godot.SentrySdk so test sources can
    // reference it without pulling in the full Sentry.Godot library (which
    // depends on Godot).
    public const string GodotSdkStub = """
        namespace Sentry.Godot;
        public static class SentrySdk
        {
            public static void StartSession() { }
            public static void EndSession() { }
            public static Sentry.SentryId CaptureMessage(string message) => default;
        }
        """;
}
