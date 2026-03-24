using System;
using System.Runtime.InteropServices;
using Sentry.Godot.Internal;

namespace Sentry.Godot.ExceptionHandling;

/// <summary>
/// Cross-platform OS thread ID retrieval.
/// </summary>
internal static class NativeThreadId {
	public static long GetCurrentThreadId() {
		if (OperatingSystem.IsWindows())
			return Windows_GetCurrentThreadId();

		if (OperatingSystem.IsLinux() || OperatingSystem.IsAndroid())
			return Linux_gettid();

		if (OperatingSystem.IsMacOS() || OperatingSystem.IsIOS()) {
			Macos_pthread_threadid_np(IntPtr.Zero, out ulong tid);
			return (long)tid;
		}

		// NOTE: Exporting for Web with C# is currently not supported in Godot 4.

		GodotLog.Error("Failed to get native thread ID for .NET managed thread.");
		return Environment.CurrentManagedThreadId;
	}

	[DllImport("kernel32.dll", EntryPoint = "GetCurrentThreadId")]
	private static extern uint Windows_GetCurrentThreadId();

	[DllImport("libc", EntryPoint = "gettid")]
	private static extern int Linux_gettid();

	// pthread_threadid_np(pthread_t thread, uint64_t *tid)
	// Pass IntPtr.Zero for current thread.
	[DllImport("libSystem.B.dylib", EntryPoint = "pthread_threadid_np")]
	private static extern int Macos_pthread_threadid_np(IntPtr thread, out ulong tid);
}
