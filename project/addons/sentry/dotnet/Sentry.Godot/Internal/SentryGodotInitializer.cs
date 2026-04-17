using System;
using Sentry.Godot.Interop;

namespace Sentry.Godot.Internal;

/// <summary>
/// Handles automatic initialization of the .NET Sentry SDK.
/// Called from [ModuleInitializer] in SentryAutoInit.cs, which is compiled into the user's assembly.
/// </summary>
public static class SentryGodotInitializer
{
    public static void AutoInit()
    {
        if (Environment.GetEnvironmentVariable("SENTRY_GODOT_EDITOR_HINT") == "1")
        {
            // Skip in the Godot editor process.
            Console.WriteLine("SentryGodotInitializer: Skipping initialization in Godot editor process.");
            return;
        }

        NativeBridge.RegisterDotnetInit();
        if (NativeBridge.IsEnabled())
        {
            GodotLog.Debug("Native is already initialized, proceeding with .NET initialization.");
            SentrySdk.InitFromNative();
        }
    }
}
