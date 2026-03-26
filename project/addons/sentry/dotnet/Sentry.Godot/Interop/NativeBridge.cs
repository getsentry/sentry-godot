using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace Sentry.Godot.Interop;

// <summary>
// Handles native layer SDK operations, such as initialize, scope syncing ops, etc.
// </summary>
internal static partial class NativeBridge {
	internal const string Lib = "sentry-godot";
	private static bool _initialized;

	[ModuleInitializer]
	[System.Diagnostics.CodeAnalysis.SuppressMessage("Usage", "CA2255")]
	internal static void Init() {
		if (_initialized)
			return;
		_initialized = true;

		// Library path set by GDExtension via env var before .NET started
		var libPath = System.Environment.GetEnvironmentVariable("SENTRY_GODOT_LIB_PATH");
		if (string.IsNullOrEmpty(libPath)) {
			Console.WriteLine("[SentryBridge] WARNING: SENTRY_GODOT_LIB_PATH not set");
			return;
		}

		// Resolve actual GDExtension library path at runtime
		NativeLibrary.SetDllImportResolver(typeof(NativeBridge).Assembly, (name, asm, path) => {
			if (name == Lib) {
				return NativeLibrary.Load(libPath);
			}
			return IntPtr.Zero;
		});
	}

	private const int InteropSmallStringLen = 64;
	private const int SizeOfChar32 = 4;

	[StructLayout(LayoutKind.Sequential)]
	private struct InteropString {
		public IntPtr Ptr;
		public long Len;

		public readonly unsafe string? ToManaged() {
			var len32 = (int)Len;
			return len32 > 0 ? Encoding.UTF32.GetString((byte *)Ptr, len32 * 4) : null;
		}
	}

	[LibraryImport(Lib)]
	private static unsafe partial void csharp_interop_sdk_set_tag(
			char *key, int keyLen, char *value, int valueLen);

	public static unsafe void SetTag(string key, string value) {
		fixed(char *keyPtr = key)
				fixed(char *valPtr = value) {
			csharp_interop_sdk_set_tag(keyPtr, key.Length, valPtr, value.Length);
		}
	}

	[LibraryImport(Lib)]
	private static unsafe partial InteropString csharp_interop_detect_environment();

	public static unsafe string? DetectEnvironment() {
		return csharp_interop_detect_environment().ToManaged();
	}

	[LibraryImport(Lib, StringMarshalling = StringMarshalling.Utf8)]
	private static partial void csharp_interop_set_trace(string traceId, string spanId);

	public static unsafe void SetTrace(string traceId, string parentSpanId) {
		csharp_interop_set_trace(traceId, parentSpanId);
	}

	// Returns compile-time literal - don't free!
	[LibraryImport(Lib)]
	private static partial IntPtr csharp_interop_get_sdk_version();

	public static string GetSdkVersion() {
		return Marshal.PtrToStringUTF8(csharp_interop_get_sdk_version()) ?? "0.0.0";
	}
}
