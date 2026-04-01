using System;
using System.Runtime.InteropServices;
using System.Text;
using Sentry.Godot.Internal;
using Godot;
using Godot.Collections;

namespace Sentry.Godot.Interop;

/// <summary>
/// Handles native layer SDK operations, such as initialize, scope syncing ops, etc.
/// </summary>
internal static partial class NativeBridge {
	internal const string Lib = "sentry-godot";
	private static readonly GodotObject _nativeSdk = Engine.GetSingleton(StringNames.SentrySDK);

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

	[StructLayout(LayoutKind.Sequential)]
	private struct OptionsData {
		public GodotStringHandle dsn;
		public GodotStringHandle release;
		public GodotStringHandle dist;
		public GodotStringHandle environment;
		public byte debug;
		public int diagnostic_level;
		public float sample_rate;
		public int max_breadcrumbs;
		public int shutdown_timeout_ms;
		public byte send_default_pii;
		public byte enable_logs;
		public byte attach_log;
		public byte attach_scene_tree;
		public byte attach_screenshot;
		public int screenshot_level;
		public byte app_hang_tracking;
		public int app_hang_timeout_sec;
		public byte logger_enabled;
		public byte logger_include_source;
		public byte logger_include_variables;
		public byte logger_messages_as_breadcrumbs;
		public int logger_event_mask;
		public int logger_breadcrumb_mask;
		public byte enable_metrics;
	}

	[LibraryImport(Lib)]
	private static unsafe partial OptionsData csharp_interop_get_options();

	[LibraryImport(Lib)]
	private static unsafe partial OptionsData csharp_interop_get_options_defaults();

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

	/// <remarks>
	/// Returns compile-time literal - don't free!
	/// </remarks>
	[LibraryImport(Lib)]
	private static partial IntPtr csharp_interop_get_sdk_version();

	public static string GetSdkVersion() {
		return Marshal.PtrToStringUTF8(csharp_interop_get_sdk_version()) ?? "0.0.0";
	}
}
