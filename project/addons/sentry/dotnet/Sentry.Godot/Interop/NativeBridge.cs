using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

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
	private static unsafe partial int csharp_interop_detect_environment(
			uint *buffer, int bufferCodepoints);

	public static unsafe string? DetectEnvironment() {
		uint *buffer = stackalloc uint[InteropSmallStringLen];
		int len = csharp_interop_detect_environment(buffer, InteropSmallStringLen);
		if (len <= 0) {
			return null;
		}
		return System.Text.Encoding.UTF32.GetString((byte *)buffer, len * SizeOfChar32);
	}

	[LibraryImport(Lib, StringMarshalling = StringMarshalling.Utf8)]
	private static partial void csharp_interop_set_trace(string traceId, string spanId);

	public static unsafe void SetTrace(string traceId, string parentSpanId) {
		csharp_interop_set_trace(traceId, parentSpanId);
	}
}
