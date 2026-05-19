using System;
using System.Runtime.InteropServices;
using Sentry.Godot.Internal;
using Sentry.Godot.Interop;

namespace Sentry.Godot.ExceptionHandling;

/// <summary>
/// Cross-platform OS thread ID retrieval.
/// </summary>
internal static partial class NativeThreadId
{
    public static long GetCurrentThreadId()
    {
        if (OperatingSystem.IsWindows())
        {
            return Windows_GetCurrentThreadId();
        }

        if (OperatingSystem.IsLinux() || NativeBridge.IsAndroid())
        {
            return Linux_gettid();
        }

        if (OperatingSystem.IsMacOS() || OperatingSystem.IsIOS())
        {
            Macos_pthread_threadid_np(IntPtr.Zero, out ulong tid);
            return (long)tid;
        }

        // NOTE: Exporting for Web with C# is currently not supported in Godot 4.

        throw new PlatformNotSupportedException("NativeThreadId used on unsupported platform");
    }

    [LibraryImport("kernel32.dll", EntryPoint = "GetCurrentThreadId")]
    private static partial uint Windows_GetCurrentThreadId();

    [LibraryImport("libc", EntryPoint = "gettid")]
    private static partial int Linux_gettid();

    // pthread_threadid_np(pthread_t thread, uint64_t *tid)
    // Pass IntPtr.Zero for current thread.
    [LibraryImport("libSystem.B.dylib", EntryPoint = "pthread_threadid_np")]
    private static partial int Macos_pthread_threadid_np(IntPtr thread, out ulong tid);
}
