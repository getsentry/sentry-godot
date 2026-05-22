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

        var libPath = Environment.GetEnvironmentVariable("SENTRY_GODOT_LIB_PATH");

        NativeLibrary.SetDllImportResolver(typeof(NativeLibResolver).Assembly, (name, asm, path) =>
        {
            if (name != NativeBridge.Lib)
            {
                return IntPtr.Zero;
            }
            if (!string.IsNullOrEmpty(libPath) && NativeLibrary.TryLoad(libPath, out var handle))
            {
                return handle;
            }

            // Fallback to RTLD_DEFAULT: takes effect on iOS where env var approach doesn't work.
            return NativeLibrary.GetMainProgramHandle();
        });
    }
}
