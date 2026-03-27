using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using Sentry.Godot.Internal;
using Godot;
using Godot.Collections;

namespace Sentry.Godot.Interop;

// <summary>
// Handles native layer SDK operations, such as initialize, scope syncing ops, etc.
// </summary>
internal static partial class NativeBridge {
	internal const string Lib = "sentry-godot";
	private static bool _initialized;
	private static GodotObject _nativeSdk = Engine.GetSingleton(StringNames.SentrySDK);

	[ModuleInitializer]
	[System.Diagnostics.CodeAnalysis.SuppressMessage("Usage", "CA2255")]
	internal static void Init() {
		// TODO: probably remove
		if (_initialized)
			return;
		_initialized = true;

		// Library path set by GDExtension via env var before .NET started
		var libPath = System.Environment.GetEnvironmentVariable("SENTRY_GODOT_LIB_PATH");
		if (string.IsNullOrEmpty(libPath)) {
			GodotLog.Error("Internal: SENTRY_GODOT_LIB_PATH not set.");
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
	private static partial void csharp_interop_string_free(IntPtr handle);

	[StructLayout(LayoutKind.Sequential)]
	private struct GodotStringHandle {
		public IntPtr Ptr;
		public long Len;
		public IntPtr Handle;

		public readonly unsafe string? TakeString() {
			if (Ptr == IntPtr.Zero) {
				return null;
			}
			var s = Encoding.UTF32.GetString((byte *)Ptr, (int)Len * SizeOfChar32);
			csharp_interop_string_free(Handle);
			return s;
		}
	}

	[LibraryImport(Lib)]
	private static unsafe partial GodotStringHandle csharp_interop_detect_environment();

	public static unsafe string? DetectEnvironment() {
		return csharp_interop_detect_environment().TakeString();
	}

	public static void AddBreadcrumb(Breadcrumb breadcrumb) {
		var crumb = ClassDB.ClassCallStatic(StringNames.SentryBreadcrumb, StringNames.create).AsGodotObject();

		if (breadcrumb.Message is not null) {
			crumb.Set(StringNames.message, breadcrumb.Message);
		}
		if (breadcrumb.Category is not null) {
			crumb.Set(StringNames.category, breadcrumb.Category);
		}
		if (breadcrumb.Type is not null) {
			crumb.Set(StringNames.type, breadcrumb.Type);
		}
		if (breadcrumb.Data is not null) {
			var data = new Dictionary();
			foreach (var kv in breadcrumb.Data) {
				data[kv.Key] = kv.Value;
			}
			crumb.Set(StringNames.data, data);
		}

		crumb.Set(StringNames.level, breadcrumb.Level switch {
			BreadcrumbLevel.Debug => 0,
			BreadcrumbLevel.Info => 1,
			BreadcrumbLevel.Warning => 2,
			BreadcrumbLevel.Error => 3,
			BreadcrumbLevel.Fatal => 4,
			_ => 1,
		});

		_nativeSdk.Call(StringNames.add_breadcrumb, crumb);
	}

	[LibraryImport(Lib)]
	private static unsafe partial void csharp_interop_sdk_set_tag(
			char *key, int keyLen, char *value, int valueLen);

	public static unsafe void SetTag(string key, string value) {
		fixed(char *keyPtr = key) fixed(char *valPtr = value) {
			csharp_interop_sdk_set_tag(keyPtr, key.Length, valPtr, value.Length);
		}
	}

	[LibraryImport(Lib)]
	private static unsafe partial void csharp_interop_sdk_remove_tag(
			char *key, int keyLen);

	public static unsafe void RemoveTag(string key) {
		fixed(char *keyPtr = key) {
			csharp_interop_sdk_remove_tag(keyPtr, key.Length);
		}
	}

	public static unsafe void SetUser(SentryUser? user) {
		if (user is null) {
			_nativeSdk.Call(StringNames.remove_user);
		} else {
			var nativeUser = ClassDB.Instantiate(StringNames.SentryUser).AsGodotObject();
			nativeUser.Set(StringNames.username, user.Username ?? "");
			nativeUser.Set(StringNames.email, user.Email ?? "");
			nativeUser.Set(StringNames.id, user.Id ?? "");
			nativeUser.Set(StringNames.ip_address, user.IpAddress ?? "");
			_nativeSdk.Call(StringNames.set_user, nativeUser);
		}
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
