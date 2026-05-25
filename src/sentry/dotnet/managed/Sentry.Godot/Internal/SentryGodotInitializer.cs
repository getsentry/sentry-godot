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
        // Module initializers run whenever the user assembly is loaded, including by tooling such as project export
        // or IDE script analysis, where the Sentry GDExtension may not be loaded.
        if (!NativeBridge.IsNativeAvailable())
        {
            Console.WriteLine("SentryGodotInitializer: Sentry GDExtension not loaded in this process; skipping. If you see this in a running game (not during project export or IDE script analysis), check Godot's log for extension load failures.");
            return;
        }

        NativeBridge.RegisterManagedFunctions();
        if (NativeBridge.IsEnabled())
        {
            GodotLog.Debug("Native is already initialized, proceeding with .NET initialization.");
            SentrySdk.InitFromNative();
        }
    }
}
