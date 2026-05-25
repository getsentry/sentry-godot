using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Sentry.Godot.Interop;

/// <summary>
/// Resolves the sentry-godot GDExtension library at runtime.
/// </summary>
internal static class NativeLibResolver
{
    [ModuleInitializer]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Usage", "CA2255")]
    internal static void Init()
    {
        NativeLibrary.SetDllImportResolver(typeof(NativeLibResolver).Assembly, (name, asm, path) =>
            name == NativeBridge.Lib ? Resolve() : IntPtr.Zero);
    }

    /// <summary>
    /// Returns the GDExtension library handle for P/Invokes and the availability probe.
    /// </summary>
    internal static IntPtr Resolve()
    {
        var libPath = Environment.GetEnvironmentVariable("SENTRY_GODOT_LIB_PATH");
        if (!string.IsNullOrEmpty(libPath) && NativeLibrary.TryLoad(libPath, out var handle))
        {
            return handle;
        }
        // Fallback to RTLD_DEFAULT: takes effect on iOS where env var approach doesn't work.
        return NativeLibrary.GetMainProgramHandle();
    }
}
