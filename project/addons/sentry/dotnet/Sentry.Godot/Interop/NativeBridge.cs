using System;
using System.Runtime.InteropServices;
using System.Text;
using Sentry.Godot.Internal;

namespace Sentry.Godot.Interop;

// NOTE: It's important to keep this file free of GodotSharp references.
//       Godot-provided bindings are not safe to use during early init path, especially on Android.

/// <summary>
/// Handles native layer SDK operations, such as initialize, scope syncing ops, etc.
/// </summary>
internal static partial class NativeBridge {
	internal const string Lib = "sentry-godot";

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
	private struct NativeOptions {
		public GodotStringHandle dsn;
		public GodotStringHandle release;
		public GodotStringHandle dist;
		public GodotStringHandle environment;
		public byte debug;
		public int diagnostic_level;
		public double sample_rate;
		public int max_breadcrumbs;
		public double shutdown_timeout_ms;
		public byte send_default_pii;
		public byte enable_logs;
		public byte attach_log;
		public byte attach_scene_tree;
		public byte attach_screenshot;
		public int screenshot_level;
		public byte app_hang_tracking;
		public double app_hang_timeout_sec;
		public byte logger_enabled;
		public byte logger_include_source;
		public byte logger_include_variables;
		public byte logger_messages_as_breadcrumbs;
		public int logger_event_mask;
		public int logger_breadcrumb_mask;
		public byte enable_metrics;
	}

	[StructLayout(LayoutKind.Sequential)]
	private unsafe struct ManagedOptions {
		public char *dsn;
		public int dsn_len;
		public char *release;
		public int release_len;
		public char *dist;
		public int dist_len;
		public char *environment;
		public int environment_len;
		public byte debug;
		public int diagnostic_level;
		public double sample_rate;
		public int max_breadcrumbs;
		public double shutdown_timeout_ms;
		public byte send_default_pii;
		public byte enable_logs;
		public byte attach_log;
		public byte attach_scene_tree;
		public byte attach_screenshot;
		public int screenshot_level;
		public byte app_hang_tracking;
		public double app_hang_timeout_sec;
		public byte logger_enabled;
		public byte logger_include_source;
		public byte logger_include_variables;
		public byte logger_messages_as_breadcrumbs;
		public int logger_event_mask;
		public int logger_breadcrumb_mask;
		public byte enable_metrics;
	}

	[LibraryImport(Lib)]
	private static unsafe partial NativeOptions csharp_interop_get_options();

	[LibraryImport(Lib)]
	private static unsafe partial NativeOptions csharp_interop_get_options_defaults();

	private static void ApplyNativeOptions(NativeOptions data, SentryGodotOptions opts) {
		opts.Dsn = data.dsn.TakeString() ?? "";
		opts.Release = data.release.TakeString();
		opts.Distribution = data.dist.TakeString();
		opts.Environment = data.environment.TakeString();
		opts.Debug = data.debug != 0;
		opts.DiagnosticLevel = (SentryLevel)data.diagnostic_level;
		opts.SampleRate = (float)data.sample_rate;
		opts.MaxBreadcrumbs = data.max_breadcrumbs;
		opts.ShutdownTimeout = TimeSpan.FromMilliseconds(data.shutdown_timeout_ms);
		opts.SendDefaultPii = data.send_default_pii != 0;
		opts.EnableLogs = data.enable_logs != 0;
		opts.AttachLog = data.attach_log != 0;
		opts.AttachSceneTree = data.attach_scene_tree != 0;
		opts.AttachScreenshot = data.attach_screenshot != 0;
		opts.ScreenshotLevel = (SentryLevel)data.screenshot_level;
		opts.AppHangTracking = data.app_hang_tracking != 0;
		opts.AppHangTimeout = TimeSpan.FromSeconds(data.app_hang_timeout_sec);
		opts.LoggerEnabled = data.logger_enabled != 0;
		opts.LoggerIncludeSource = data.logger_include_source != 0;
		opts.LoggerIncludeVariables = data.logger_include_variables != 0;
		opts.LoggerMessagesAsBreadcrumbs = data.logger_messages_as_breadcrumbs != 0;
		opts.LoggerEventMask = (SentryGodotOptions.GodotErrorMask)data.logger_event_mask;
		opts.LoggerBreadcrumbMask = (SentryGodotOptions.GodotErrorMask)data.logger_breadcrumb_mask;
		opts.Experimental.EnableMetrics = data.enable_metrics != 0;
	}

	public static void ApplyNativeOptions(SentryGodotOptions opts) {
		ApplyNativeOptions(csharp_interop_get_options(), opts);
	}

	public static void ApplyNativeOptionsDefaults(SentryGodotOptions opts) {
		ApplyNativeOptions(csharp_interop_get_options_defaults(), opts);
	}

	[LibraryImport(Lib)]
	private static unsafe partial void csharp_interop_register_dotnet_init(
			delegate *unmanaged[Cdecl]<void> fn);

	[UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
	private static void DotnetInitCallback() {
		SentrySdk.Init();
	}

	public static unsafe void RegisterDotnetInit() {
		csharp_interop_register_dotnet_init(&DotnetInitCallback);
		GodotLog.Debug(".NET layer is ready for SDK init.");
	}

	[LibraryImport(Lib)]
	private static unsafe partial GodotStringHandle csharp_interop_detect_environment();

	public static unsafe string? DetectEnvironment() {
		return csharp_interop_detect_environment().TakeString();
	}

	[LibraryImport(Lib)]
	private static unsafe partial GodotStringHandle csharp_interop_get_app_name();

	[LibraryImport(Lib)]
	private static unsafe partial GodotStringHandle csharp_interop_get_app_version();

	public static string GetAppName() {
		return csharp_interop_get_app_name().TakeString() ?? "";
	}

	public static string GetAppVersion() {
		return csharp_interop_get_app_version().TakeString() ?? "";
	}

	[LibraryImport(Lib)]
	private static unsafe partial byte csharp_interop_sdk_is_enabled();

	public static bool IsEnabled() {
		return csharp_interop_sdk_is_enabled() != 0;
	}

	[LibraryImport(Lib)]
	private static unsafe partial byte csharp_interop_is_debugger_active();

	public static bool IsDebuggerActive() {
		return csharp_interop_is_debugger_active() != 0;
	}

	[LibraryImport(Lib)]
	private static unsafe partial void csharp_interop_sdk_init(ManagedOptions opts);

	public static unsafe void InitNativeSdk(SentryGodotOptions opts) {
		var dsn = opts.Dsn ?? "";
		var release = opts.Release ?? "";
		var dist = opts.Distribution ?? "";
		var env = opts.Environment ?? "";

		fixed(char *dsnPtr = dsn)
				fixed(char *relPtr = release)
						fixed(char *distPtr = dist)
								fixed(char *envPtr = env) {
			var managed = new ManagedOptions {
				dsn = dsnPtr,
				dsn_len = dsn.Length,
				release = relPtr,
				release_len = release.Length,
				dist = distPtr,
				dist_len = dist.Length,
				environment = envPtr,
				environment_len = env.Length,
				debug = (byte)(opts.Debug ? 1 : 0),
				diagnostic_level = (int)opts.DiagnosticLevel,
				sample_rate = opts.SampleRate ?? 1.0,
				max_breadcrumbs = opts.MaxBreadcrumbs,
				shutdown_timeout_ms = opts.ShutdownTimeout.TotalMilliseconds,
				send_default_pii = (byte)(opts.SendDefaultPii ? 1 : 0),
				enable_logs = (byte)(opts.EnableLogs ? 1 : 0),
				attach_log = (byte)(opts.AttachLog ? 1 : 0),
				attach_scene_tree = (byte)(opts.AttachSceneTree ? 1 : 0),
				attach_screenshot = (byte)(opts.AttachScreenshot ? 1 : 0),
				screenshot_level = (int)opts.ScreenshotLevel,
				app_hang_tracking = (byte)(opts.AppHangTracking ? 1 : 0),
				app_hang_timeout_sec = opts.AppHangTimeout.TotalSeconds,
				logger_enabled = (byte)(opts.LoggerEnabled ? 1 : 0),
				logger_include_source = (byte)(opts.LoggerIncludeSource ? 1 : 0),
				logger_include_variables = (byte)(opts.LoggerIncludeVariables ? 1 : 0),
				logger_messages_as_breadcrumbs = (byte)(opts.LoggerMessagesAsBreadcrumbs ? 1 : 0),
				logger_event_mask = (int)opts.LoggerEventMask,
				logger_breadcrumb_mask = (int)opts.LoggerBreadcrumbMask,
				enable_metrics = (byte)(opts.Experimental.EnableMetrics ? 1 : 0),
			};
			csharp_interop_sdk_init(managed);
		}
	}

	[LibraryImport(Lib)]
	private static unsafe partial void csharp_interop_sdk_add_breadcrumb(
			char *message, int messageLen,
			char *category, int categoryLen,
			char *type, int typeLen,
			int level,
			ManagedStringMap data);

	public static unsafe void AddBreadcrumb(Breadcrumb breadcrumb) {
		var msg = breadcrumb.Message ?? "";
		var cat = breadcrumb.Category ?? "";
		var typ = breadcrumb.Type ?? "";
		int level = breadcrumb.Level switch {
			BreadcrumbLevel.Debug => 0,
			BreadcrumbLevel.Info => 1,
			BreadcrumbLevel.Warning => 2,
			BreadcrumbLevel.Error => 3,
			BreadcrumbLevel.Fatal => 4,
			_ => 1,
		};

		ManagedStringMap.Marshall(breadcrumb.Data, map => {
			fixed(char *msgPtr = msg)
					fixed(char *catPtr = cat)
							fixed(char *typPtr = typ) {
				csharp_interop_sdk_add_breadcrumb(
						msgPtr, msg.Length,
						catPtr, cat.Length,
						typPtr, typ.Length,
						level, map);
			}
		});
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

	[LibraryImport(Lib)]
	private static unsafe partial void csharp_interop_sdk_remove_user();

	[LibraryImport(Lib)]
	private static unsafe partial void csharp_interop_sdk_set_user(
			char *username, int usernameLen, char *email, int emailLen, char *id, int idLen, char *ipAddress, int ipAddressLen);

	public static unsafe void SetUser(SentryUser? user) {
		if (user is null) {
			csharp_interop_sdk_remove_user();
		} else {
			fixed(char *usernamePtr = user.Username)
					fixed(char *emailPtr = user.Email)
							fixed(char *idPtr = user.Id)
									fixed(char *ipAddressPtr = user.IpAddress) {
				csharp_interop_sdk_set_user(
						usernamePtr, user.Username?.Length ?? 0,
						emailPtr, user.Email?.Length ?? 0,
						idPtr, user.Id?.Length ?? 0,
						ipAddressPtr, user.IpAddress?.Length ?? 0);
			}
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

	[LibraryImport(Lib)]
	private static unsafe partial void csharp_interop_log(int level, char *msg, int len);

	public static unsafe void Log(SentryLevel level, string message) {
		fixed(char *ptr = message) {
			csharp_interop_log((int)level, ptr, message.Length);
		}
	}
}
