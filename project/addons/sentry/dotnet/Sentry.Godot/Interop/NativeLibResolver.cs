using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Sentry.Godot.Interop;

/// <summary>
/// Resolves the sentry-godot GDExtension library at runtime.
/// </summary>
internal static class NativeLibResolver
{
    private static bool _initialized;

    [ModuleInitializer]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Usage", "CA2255")]
    internal static void Init()
    {
        if (_initialized)
        {
            return;
        }
        _initialized = true;

        // Library path set by GDExtension via env var before .NET started
        var libPath = Environment.GetEnvironmentVariable("SENTRY_GODOT_LIB_PATH");
        if (string.IsNullOrEmpty(libPath))
        {
            Console.Error.WriteLine("Sentry: ERROR: SENTRY_GODOT_LIB_PATH not set.");
            return;
        }

        NativeLibrary.SetDllImportResolver(typeof(NativeLibResolver).Assembly, (name, asm, path) =>
        {
            if (name == NativeBridge.Lib)
            {
                return NativeLibrary.Load(libPath);
            }
            return IntPtr.Zero;
        });
    }
}
