using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Sentry.Godot.Interop;

/// <summary>
/// Resolves the sentry-godot GDExtension library at runtime.
/// </summary>
internal static class NativeLibResolver
{
    private const string WindowsLibPrefix = "libsentry.windows";

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

        NativeLibrary.SetDllImportResolver(typeof(NativeLibResolver).Assembly, (name, asm, path) =>
        {
            if (name != NativeBridge.Lib)
            {
                return IntPtr.Zero;
            }

            if (OperatingSystem.IsWindows())
            {
                // Windows: no RTLD_DEFAULT equivalent.
                // Enumerate loaded modules and find the proper one.
                var libPath = FindLoadedLibraryPath(WindowsLibPrefix);
                return libPath is not null && NativeLibrary.TryLoad(libPath, out var handle)
                    ? handle
                    : IntPtr.Zero;
            }
            else
            {
                // Unix: exports are reachable via RTLD_DEFAULT.
                return NativeLibrary.GetMainProgramHandle();

            }

        });
    }

    private static string? FindLoadedLibraryPath(string prefix)
    {
        foreach (ProcessModule module in Process.GetCurrentProcess().Modules)
        {
            var name = module.ModuleName;
            if (name is not null && name.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            {
                return module.FileName;
            }
        }
        return null;
    }
}
