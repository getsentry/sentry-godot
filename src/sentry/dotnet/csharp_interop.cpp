#include "sentry/dotnet/csharp_interop.h"

#include "gen/sdk_version.gen.h"
#include "sentry/dotnet/dotnet_scope_observer.h"
#include "sentry/dotnet/process_default_attachments.h"
#include "sentry/environment.h"
#include "sentry/logging/print.h"
#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_event.h"
#include "sentry/sentry_sdk.h"
#include "sentry/sentry_user.h"

#ifdef SDK_COCOA
#include "sentry/cocoa/cocoa_debug_images.h"
#endif

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/type_info.hpp>

#include <cstring>

#ifdef _WIN32
#define CSHARP_EXPORT __declspec(dllexport)
#else
#define CSHARP_EXPORT __attribute__((visibility("default")))
#endif

using namespace sentry;

extern "C" {

// Native-owned string handle for passing Godot Strings across the interop boundary.
// C# must call csharp_interop_string_free() after reading the data.
// Must match layout of GodotStringHandle in NativeBridge.cs.
struct GodotStringHandle {
	const char32_t *ptr;
	int64_t len;
	void *handle; // Opaque: heap-allocated String* that owns the refcount.
};

GodotStringHandle _make_handle(const String &p_str) {
	String *owned = memnew(String(p_str));
	return { owned->ptr(), owned->length(), (void *)owned };
}

CSHARP_EXPORT void csharp_interop_string_free(void *p_handle) {
	if (p_handle) {
		memdelete((String *)p_handle);
	}
}

// Managed-owned string map for passing Dictionary<string, string> across P/Invoke.
// C# builds and pins this; native reads it synchronously during the call.
// Buffer layout: key1+val1+key2+val2... concatenated as UTF-16.
// Lengths array: key1_len, val1_len, key2_len, val2_len, ... (2*pair_count entries).
struct ManagedStringMap {
	const char16_t *buffer;
	const int32_t *lengths;
	int32_t pair_count;
};

// Decode a ManagedStringMap into a Godot Dictionary (String -> String).
static Dictionary _managed_string_map_to_dictionary(const ManagedStringMap &map) {
	Dictionary dict;
	if (map.pair_count <= 0 || map.buffer == nullptr || map.lengths == nullptr) {
		return dict;
	}
	const char16_t *ptr = map.buffer;
	for (int32_t i = 0; i < map.pair_count; i++) {
		int32_t key_len = map.lengths[i * 2];
		int32_t val_len = map.lengths[i * 2 + 1];
		String key = String::utf16(ptr, key_len);
		ptr += key_len;
		String val = String::utf16(ptr, val_len);
		ptr += val_len;
		dict[key] = val;
	}
	return dict;
}

// Managed functions that are called from native layer.
// Must match ManagedFunctions struct in NativeBridge.cs.
struct ManagedFunctions {
	void (*init)();
	void (*close)();
	void (*logger_error)(const char16_t *code, int32_t code_len, const char16_t *file, int32_t file_len);
	void (*add_breadcrumb)(const char16_t *message, int32_t message_len, const char16_t *category, int32_t category_len, const char16_t *type, int32_t type_len, int32_t level);
	void (*set_tag)(const char16_t *name, int32_t name_len, const char16_t *value, int32_t value_len);
	void (*remove_tag)(const char16_t *name, int32_t name_len);
	void (*set_user)(const char16_t *id, int32_t id_len, const char16_t *username, int32_t username_len, const char16_t *email, int32_t email_len, const char16_t *ip, int32_t ip_len);
	void (*remove_user)();
	uint8_t (*process_native_event)(void *event_handle); // Returns 1 to keep, 0 to discard.
};

static ManagedFunctions s_managed_funcs = {};

// Flags indicating which native-layer hooks are implemented in managed code.
// Passed in ManagedOptions.defined_hooks during init to avoid crossing the managed boundary for unset hooks.
// Must match ManagedDefinedHooks in NativeBridge.cs.
enum ManagedDefinedHooks {
	DEFINED_NONE = 0,
	DEFINED_BEFORE_SEND = 1 << 0, // options.Native.SetBeforeSend
};

static BitField<ManagedDefinedHooks> s_managed_defined_hooks = DEFINED_NONE;

// Must match layout of LoggerLimitsData in NativeBridge.cs.
struct LoggerLimitsData {
	int32_t events_per_frame;
	int32_t repeated_error_window_ms;
	int32_t throttle_events;
	int32_t throttle_window_ms;
};

// Must match layout of AttachmentMeta in NativeBridge.cs.
struct AttachmentMeta {
	GodotStringHandle path;
	GodotStringHandle filename;
	GodotStringHandle content_type;
	GodotStringHandle attachment_type;
};

// Must match layout of NativeOptions in NativeBridge.cs.
struct NativeOptions {
	LoggerLimitsData logger_limits;

	// NOTE: Strings are native-owned, but C# should "take" all strings or it will leak.
	GodotStringHandle dsn;
	GodotStringHandle release;
	GodotStringHandle dist;
	GodotStringHandle environment;

	// Value types
	uint8_t debug;
	int32_t diagnostic_level;
	double sample_rate;
	int32_t max_breadcrumbs;
	double shutdown_timeout_ms;
	uint8_t send_default_pii;
	uint8_t enable_logs;

	// Godot-specific
	uint8_t attach_log;
	uint8_t attach_scene_tree;
	uint8_t attach_screenshot;
	int32_t screenshot_level;
	uint8_t enable_app_hang_tracking;
	int32_t app_hang_timeout_ms;

	// Logger
	uint8_t logger_enabled;
	uint8_t logger_include_source_context;
	uint8_t logger_include_variables;
	int32_t logger_event_mask;
	int32_t logger_breadcrumb_mask;
	int32_t logger_log_mask;

	// Experimental
	uint8_t enable_metrics;

	// Android
	uint8_t android_enable_anr_detection;
	int32_t android_anr_timeout_interval_ms;
	uint8_t android_attach_anr_thread_dump;
};

// Generic array handle for returning native-allocated arrays across the interop boundary.
// C# casts "ptr" to the concrete element type and must free it via csharp_interop_free_array().
struct NativeArray {
	void *ptr;
	int32_t count;
};

CSHARP_EXPORT void csharp_interop_free_array(void *p_array) {
	if (p_array) {
		// Equivalent to memdelete_arr() for trivially-destructible types.
		// memdelete_arr() can't be used directly here because the call site only has a void*.
		Memory::free_static(p_array, true);
	}
}

// Managed-owned options for passing C# options to native.
// C# pins strings; native reads synchronously. No free needed.
// Must match layout of ManagedOptions in NativeBridge.cs.
struct ManagedOptions {
	LoggerLimitsData logger_limits;

	const char16_t *dsn;
	int32_t dsn_len;
	const char16_t *release;
	int32_t release_len;
	const char16_t *dist;
	int32_t dist_len;
	const char16_t *environment;
	int32_t environment_len;

	uint8_t debug;
	int32_t diagnostic_level;
	double sample_rate;
	int32_t max_breadcrumbs;
	double shutdown_timeout_ms;
	uint8_t send_default_pii;
	uint8_t enable_logs;

	uint8_t attach_log;
	uint8_t attach_scene_tree;
	uint8_t attach_screenshot;
	int32_t screenshot_level;
	uint8_t enable_app_hang_tracking;
	int32_t app_hang_timeout_ms;

	uint8_t logger_enabled;
	uint8_t logger_include_source_context;
	uint8_t logger_include_variables;
	int32_t logger_event_mask;
	int32_t logger_breadcrumb_mask;
	int32_t logger_log_mask;

	uint8_t enable_metrics;

	// Flags indicating which native-layer hooks are implemented in managed code; see ManagedDefinedHooks.
	uint32_t defined_hooks;

	uint8_t android_enable_anr_detection;
	int32_t android_anr_timeout_interval_ms;
	uint8_t android_attach_anr_thread_dump;
};

struct NativeTraceContext {
	GodotStringHandle trace_id;
	GodotStringHandle parent_span_id;
};

static void _apply_managed_options(const ManagedOptions &data, Ref<SentryOptions> options) {
	Ref<SentryLoggerLimits> logger_limits = options->get_godot_logger()->get_limits();
	logger_limits->set_events_per_frame(data.logger_limits.events_per_frame);
	logger_limits->set_repeated_error_window_ms(data.logger_limits.repeated_error_window_ms);
	logger_limits->set_throttle_events(data.logger_limits.throttle_events);
	logger_limits->set_throttle_window_ms(data.logger_limits.throttle_window_ms);

	options->set_dsn(String::utf16(data.dsn, data.dsn_len));
	options->set_release(String::utf16(data.release, data.release_len));
	options->set_dist(String::utf16(data.dist, data.dist_len));
	options->set_environment(String::utf16(data.environment, data.environment_len));
	options->set_debug_enabled(data.debug);
	options->set_diagnostic_level((Level)data.diagnostic_level);
	options->set_sample_rate(data.sample_rate);
	options->set_max_breadcrumbs(data.max_breadcrumbs);
	options->set_shutdown_timeout_ms(data.shutdown_timeout_ms);
	options->set_send_default_pii(data.send_default_pii);
	options->set_enable_logs(data.enable_logs);
	options->set_attach_log(data.attach_log);
	options->set_attach_scene_tree(data.attach_scene_tree);
	options->set_attach_screenshot(data.attach_screenshot);
	options->set_screenshot_level((Level)data.screenshot_level);
	options->set_app_hang_tracking_enabled(data.enable_app_hang_tracking);
	options->set_app_hang_timeout_ms(data.app_hang_timeout_ms);
	options->get_godot_logger()->set_enabled(data.logger_enabled);
	options->get_godot_logger()->set_include_source_context(data.logger_include_source_context);
	options->get_godot_logger()->set_include_variables(data.logger_include_variables);
	options->get_godot_logger()->set_event_mask(data.logger_event_mask);
	options->get_godot_logger()->set_breadcrumb_mask(data.logger_breadcrumb_mask);
	options->get_godot_logger()->set_log_mask(data.logger_log_mask);
	options->set_enable_metrics(data.enable_metrics);
	options->get_android()->set_enable_anr_detection(data.android_enable_anr_detection);
	options->get_android()->set_anr_timeout_interval_ms(data.android_anr_timeout_interval_ms);
	options->get_android()->set_attach_anr_thread_dump(data.android_attach_anr_thread_dump);
}

void _populate_options_data(NativeOptions &r_data, const Ref<SentryOptions> &options) {
	r_data = {};

	Ref<SentryLoggerLimits> limits = options->get_godot_logger()->get_limits();
	r_data.logger_limits.events_per_frame = limits->get_events_per_frame();
	r_data.logger_limits.repeated_error_window_ms = limits->get_repeated_error_window_ms();
	r_data.logger_limits.throttle_events = limits->get_throttle_events();
	r_data.logger_limits.throttle_window_ms = limits->get_throttle_window_ms();

	r_data.dsn = _make_handle(options->get_dsn());
	r_data.release = _make_handle(options->get_release());
	r_data.dist = _make_handle(options->get_dist());
	r_data.environment = _make_handle(options->get_environment());
	r_data.debug = options->is_debug_enabled();
	r_data.diagnostic_level = options->get_diagnostic_level();
	r_data.sample_rate = options->get_sample_rate();
	r_data.max_breadcrumbs = options->get_max_breadcrumbs();
	r_data.shutdown_timeout_ms = options->get_shutdown_timeout_ms();
	r_data.send_default_pii = options->is_send_default_pii_enabled();
	r_data.enable_logs = options->get_enable_logs();
	r_data.attach_log = options->is_attach_log_enabled();
	r_data.attach_scene_tree = options->is_attach_scene_tree_enabled();
	r_data.attach_screenshot = options->is_attach_screenshot_enabled();
	r_data.screenshot_level = options->get_screenshot_level();
	r_data.enable_app_hang_tracking = options->is_app_hang_tracking_enabled();
	r_data.app_hang_timeout_ms = options->get_app_hang_timeout_ms();
	r_data.logger_enabled = options->get_godot_logger()->get_enabled();
	r_data.logger_include_source_context = options->get_godot_logger()->get_include_source_context();
	r_data.logger_include_variables = options->get_godot_logger()->get_include_variables();
	r_data.logger_event_mask = options->get_godot_logger()->get_event_mask();
	r_data.logger_breadcrumb_mask = options->get_godot_logger()->get_breadcrumb_mask();
	r_data.logger_log_mask = options->get_godot_logger()->get_log_mask();
	r_data.enable_metrics = options->get_enable_metrics();
	r_data.android_enable_anr_detection = options->get_android()->get_enable_anr_detection();
	r_data.android_anr_timeout_interval_ms = options->get_android()->get_anr_timeout_interval_ms();
	r_data.android_attach_anr_thread_dump = options->get_android()->get_attach_anr_thread_dump();
}

// *** Functions called from C#

CSHARP_EXPORT void csharp_interop_register_managed_functions(ManagedFunctions p_functions) {
	s_managed_funcs = p_functions;
}

CSHARP_EXPORT NativeOptions csharp_interop_get_options() {
	NativeOptions data;
	_populate_options_data(data, SentrySDK::get_singleton()->get_options());
	return data;
}

CSHARP_EXPORT NativeOptions csharp_interop_get_options_defaults() {
	NativeOptions data;
	_populate_options_data(data, SentryOptions::create_from_project_settings());
	return data;
}

CSHARP_EXPORT NativeArray csharp_interop_get_default_attachments() {
	NativeArray result = {};
	const Vector<Ref<SentryAttachment>> &atts = SentrySDK::get_singleton()->get_options()->get_default_attachments();
	result.count = atts.size();
	if (result.count > 0) {
		AttachmentMeta *items = memnew_arr(AttachmentMeta, result.count);
		for (int32_t i = 0; i < result.count; ++i) {
			items[i].path = _make_handle(atts[i]->get_globalized_path());
			items[i].filename = _make_handle(atts[i]->get_filename());
			items[i].content_type = _make_handle(atts[i]->get_content_type());
			items[i].attachment_type = _make_handle(atts[i]->get_attachment_type());
		}
		result.ptr = items;
	}
	return result;
}

CSHARP_EXPORT NativeTraceContext csharp_interop_get_trace_context() {
	auto ctx = SentrySDK::get_singleton()->get_trace_context();

	NativeTraceContext interop_ctx;
	interop_ctx.trace_id = _make_handle(ctx.trace_id);
	interop_ctx.parent_span_id = _make_handle(ctx.parent_span_id);

	return interop_ctx;
}

CSHARP_EXPORT GodotStringHandle csharp_interop_detect_environment() {
	return _make_handle(environment::detect_godot_environment());
}

CSHARP_EXPORT GodotStringHandle csharp_interop_get_app_name() {
	return _make_handle(ProjectSettings::get_singleton()->get_setting("application/config/name"));
}

CSHARP_EXPORT GodotStringHandle csharp_interop_get_app_version() {
	return _make_handle(ProjectSettings::get_singleton()->get_setting("application/config/version"));
}

CSHARP_EXPORT bool csharp_interop_sdk_is_enabled() {
	return SentrySDK::get_singleton()->is_enabled();
}

CSHARP_EXPORT bool csharp_interop_is_main_thread() {
	return OS::get_singleton()->get_thread_caller_id() == OS::get_singleton()->get_main_thread_id();
}

CSHARP_EXPORT bool csharp_interop_is_debugger_active() {
	return EngineDebugger::get_singleton() && EngineDebugger::get_singleton()->is_active();
}

CSHARP_EXPORT bool csharp_interop_is_android() {
#ifdef ANDROID_ENABLED
	return true;
#else
	return false;
#endif
}

// Remarks:
// Managed side needs to access assemblies for .NET stack-trace symbolication. On Android, these assemblies live inside
// the application archive, potentially inside a .pck container, so direct file access is not viable. Routing through
// FileAccess lets the engine's virtual filesystem handle packing and decompression. The Sentry SDK needs each
// assembly's PE bytes to build a debug image, but only reads two small segments: the headers at the start and the debug
// directory near the end. The assembly is exposed as a seekable handle so the managed side reads just those segments,
// without marshalling the whole file content.

// Opaque handle for an open managed assembly file.
// Must match layout of AssemblyHandle in NativeBridge.cs.
// Must be closed with csharp_interop_close_managed_assembly().
struct AssemblyHandle {
	void *handle; // Ref<FileAccess>* or nullptr if the assembly was not found
	int64_t length;
};

CSHARP_EXPORT AssemblyHandle csharp_interop_open_managed_assembly(const char16_t *p_name, int32_t p_name_len) {
	AssemblyHandle result = {};

	String name = String::utf16(p_name, p_name_len);
	name = name.get_file();
	if (!name.ends_with(".dll")) {
		name += ".dll";
	}

	String arch = Engine::get_singleton()->get_architecture_name();
	String path = String("res://.godot/mono/publish/").path_join(arch).path_join(name);

	Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ);
	if (file.is_null()) {
		return result;
	}

	result.handle = memnew(Ref<FileAccess>(file));
	result.length = file->get_length();
	return result;
}

CSHARP_EXPORT int64_t csharp_interop_read_managed_assembly(void *p_handle, int64_t p_offset, int64_t p_count, uint8_t *r_dst) {
	if (p_handle == nullptr || r_dst == nullptr || p_count <= 0) {
		return 0;
	}

	const Ref<FileAccess> &file = *static_cast<Ref<FileAccess> *>(p_handle);
	if (file.is_null()) {
		return 0;
	}

	file->seek(p_offset);
	PackedByteArray data = file->get_buffer(p_count);
	int64_t num_read = data.size();
	if (num_read > 0) {
		memcpy(r_dst, data.ptr(), num_read);
	}
	return num_read;
}

CSHARP_EXPORT void csharp_interop_close_managed_assembly(void *p_handle) {
	if (p_handle != nullptr) {
		memdelete(static_cast<Ref<FileAccess> *>(p_handle));
	}
}

#ifdef SDK_COCOA

// Mach-O debug image entry returned to C# for NativeAOT stack symbolication on iOS.
// Must match layout of CocoaDebugImageEntry in NativeBridge.cs.
struct CocoaDebugImageEntry {
	GodotStringHandle code_file;
	GodotStringHandle debug_id;
	int64_t image_address;
	int64_t image_size;
};

CSHARP_EXPORT int32_t csharp_interop_get_cocoa_debug_images(
		const int64_t *p_addresses, int32_t p_addresses_count,
		CocoaDebugImageEntry *r_entries, int32_t p_entries_capacity) {
	if (p_addresses == nullptr || p_addresses_count <= 0 || r_entries == nullptr || p_entries_capacity <= 0) {
		return 0;
	}

	Vector<sentry::cocoa::DebugImage> images = sentry::cocoa::get_debug_images(p_addresses, p_addresses_count);
	int32_t count = MIN(static_cast<int32_t>(images.size()), p_entries_capacity);
	for (int32_t i = 0; i < count; ++i) {
		const sentry::cocoa::DebugImage &image = images[i];
		CocoaDebugImageEntry &entry = r_entries[i];
		entry.code_file = _make_handle(image.code_file);
		entry.debug_id = _make_handle(image.debug_id);
		entry.image_address = image.image_address;
		entry.image_size = image.image_size;
	}
	return count;
}

#endif // SDK_COCOA

static ManagedOptions s_pending_managed_opts;

static void _apply_pending_managed_options(const Ref<SentryOptions> &options) {
	_apply_managed_options(s_pending_managed_opts, options);
}

CSHARP_EXPORT void csharp_interop_sdk_init(ManagedOptions managed_opts) {
	s_pending_managed_opts = managed_opts;
	s_managed_defined_hooks = managed_opts.defined_hooks;
	SentrySDK::get_singleton()->init(callable_mp_static(_apply_pending_managed_options));
	s_pending_managed_opts = {};
}

CSHARP_EXPORT void csharp_interop_sdk_close() {
	SentrySDK::get_singleton()->close();
}

CSHARP_EXPORT void csharp_interop_process_default_attachments(int32_t level) {
	sentry::dotnet::process_default_attachments(static_cast<sentry::Level>(level));
}

CSHARP_EXPORT void csharp_interop_sdk_add_breadcrumb(
		const char16_t *message, int32_t message_len,
		const char16_t *category, int32_t category_len,
		const char16_t *type, int32_t type_len,
		int32_t level,
		ManagedStringMap data) {
	sentry::dotnet::DotnetScopeObserver::SyncGuard guard;
	Ref<SentryBreadcrumb> crumb = SentryBreadcrumb::create(String::utf16(message, message_len));
	crumb->set_category(String::utf16(category, category_len));
	crumb->set_type(String::utf16(type, type_len));
	crumb->set_level((Level)level);
	crumb->set_data(_managed_string_map_to_dictionary(data));
	SentrySDK::get_singleton()->add_breadcrumb(crumb);
}

CSHARP_EXPORT void csharp_interop_sdk_set_tag(const char16_t *key, int32_t key_len, const char16_t *value, int32_t value_len) {
	sentry::dotnet::DotnetScopeObserver::SyncGuard guard;
	String k = String::utf16(key, key_len);
	String v = String::utf16(value, value_len);
	SentrySDK::get_singleton()->set_tag(k, v);
}

CSHARP_EXPORT void csharp_interop_sdk_remove_tag(const char16_t *key, int32_t key_len) {
	sentry::dotnet::DotnetScopeObserver::SyncGuard guard;
	String k = String::utf16(key, key_len);
	SentrySDK::get_singleton()->remove_tag(k);
}

CSHARP_EXPORT void csharp_interop_sdk_set_user(
		const char16_t *id, int32_t id_len,
		const char16_t *username, int32_t username_len,
		const char16_t *email, int32_t email_len,
		const char16_t *ip_address, int32_t ip_address_len) {
	sentry::dotnet::DotnetScopeObserver::SyncGuard guard;
	Ref<SentryUser> user;
	user.instantiate();
	user->set_id(String::utf16(id, id_len));
	user->set_username(String::utf16(username, username_len));
	user->set_email(String::utf16(email, email_len));
	user->set_ip_address(String::utf16(ip_address, ip_address_len));
	SentrySDK::get_singleton()->set_user(user);
}

CSHARP_EXPORT void csharp_interop_sdk_remove_user() {
	sentry::dotnet::DotnetScopeObserver::SyncGuard guard;
	SentrySDK::get_singleton()->remove_user();
}

CSHARP_EXPORT void csharp_interop_sdk_set_trace(const char16_t *trace_id, int32_t trace_id_len, const char16_t *parent_span_id, int32_t parent_span_id_len) {
	sentry::dotnet::DotnetScopeObserver::SyncGuard guard;
	SentrySDK::get_singleton()->set_trace(
			String::utf16(trace_id, trace_id_len),
			String::utf16(parent_span_id, parent_span_id_len));
}

CSHARP_EXPORT const char *csharp_interop_get_sdk_version() {
	return SENTRY_GODOT_SDK_VERSION;
}

CSHARP_EXPORT void csharp_interop_log(int32_t level, const char16_t *msg, int32_t len) {
	sentry::logging::print(
			static_cast<sentry::Level>(level),
			String::utf16(msg, len));
}

// *** Event accessors used by the options.Native.SetBeforeSend callback.

// The handle is the SentryEvent* passed to the managed layer; it is valid only for the duration of that call.
// The managed wrapper SentryNativeEvent reads and writes the event lazily through these functions,
// so cost is proportional to the fields touched.

static inline SentryEvent *_event_from_handle(void *p_handle) {
	return static_cast<SentryEvent *>(p_handle);
}

CSHARP_EXPORT GodotStringHandle csharp_interop_event_get_id(void *p_handle) {
	return _make_handle(_event_from_handle(p_handle)->get_id());
}

CSHARP_EXPORT GodotStringHandle csharp_interop_event_get_platform(void *p_handle) {
	return _make_handle(_event_from_handle(p_handle)->get_platform());
}

CSHARP_EXPORT GodotStringHandle csharp_interop_event_get_message(void *p_handle) {
	return _make_handle(_event_from_handle(p_handle)->get_message());
}

CSHARP_EXPORT void csharp_interop_event_set_message(void *p_handle, const char16_t *p_value, int32_t p_value_len) {
	_event_from_handle(p_handle)->set_message(String::utf16(p_value, p_value_len));
}

CSHARP_EXPORT int32_t csharp_interop_event_get_level(void *p_handle) {
	return static_cast<int32_t>(_event_from_handle(p_handle)->get_level());
}

CSHARP_EXPORT void csharp_interop_event_set_level(void *p_handle, int32_t p_level) {
	_event_from_handle(p_handle)->set_level(static_cast<sentry::Level>(p_level));
}

CSHARP_EXPORT GodotStringHandle csharp_interop_event_get_logger(void *p_handle) {
	return _make_handle(_event_from_handle(p_handle)->get_logger());
}

CSHARP_EXPORT void csharp_interop_event_set_logger(void *p_handle, const char16_t *p_value, int32_t p_value_len) {
	_event_from_handle(p_handle)->set_logger(String::utf16(p_value, p_value_len));
}

CSHARP_EXPORT GodotStringHandle csharp_interop_event_get_release(void *p_handle) {
	return _make_handle(_event_from_handle(p_handle)->get_release());
}

CSHARP_EXPORT void csharp_interop_event_set_release(void *p_handle, const char16_t *p_value, int32_t p_value_len) {
	_event_from_handle(p_handle)->set_release(String::utf16(p_value, p_value_len));
}

CSHARP_EXPORT GodotStringHandle csharp_interop_event_get_dist(void *p_handle) {
	return _make_handle(_event_from_handle(p_handle)->get_dist());
}

CSHARP_EXPORT void csharp_interop_event_set_dist(void *p_handle, const char16_t *p_value, int32_t p_value_len) {
	_event_from_handle(p_handle)->set_dist(String::utf16(p_value, p_value_len));
}

CSHARP_EXPORT GodotStringHandle csharp_interop_event_get_environment(void *p_handle) {
	return _make_handle(_event_from_handle(p_handle)->get_environment());
}

CSHARP_EXPORT void csharp_interop_event_set_environment(void *p_handle, const char16_t *p_value, int32_t p_value_len) {
	_event_from_handle(p_handle)->set_environment(String::utf16(p_value, p_value_len));
}

CSHARP_EXPORT GodotStringHandle csharp_interop_event_get_tag(void *p_handle, const char16_t *p_key, int32_t p_key_len) {
	return _make_handle(_event_from_handle(p_handle)->get_tag(String::utf16(p_key, p_key_len)));
}

CSHARP_EXPORT void csharp_interop_event_set_tag(void *p_handle, const char16_t *p_key, int32_t p_key_len, const char16_t *p_value, int32_t p_value_len) {
	_event_from_handle(p_handle)->set_tag(String::utf16(p_key, p_key_len), String::utf16(p_value, p_value_len));
}

CSHARP_EXPORT void csharp_interop_event_remove_tag(void *p_handle, const char16_t *p_key, int32_t p_key_len) {
	_event_from_handle(p_handle)->remove_tag(String::utf16(p_key, p_key_len));
}

CSHARP_EXPORT int32_t csharp_interop_event_get_exception_count(void *p_handle) {
	return _event_from_handle(p_handle)->get_exception_count();
}

CSHARP_EXPORT GodotStringHandle csharp_interop_event_get_exception_value(void *p_handle, int32_t p_index) {
	return _make_handle(_event_from_handle(p_handle)->get_exception_value(p_index));
}

CSHARP_EXPORT void csharp_interop_event_set_exception_value(void *p_handle, int32_t p_index, const char16_t *p_value, int32_t p_value_len) {
	_event_from_handle(p_handle)->set_exception_value(p_index, String::utf16(p_value, p_value_len));
}

} // extern "C"

// *** Functions called from native

namespace sentry::dotnet {

bool godot_supports_dotnet() {
	// CSharpScript only exists in a mono (.NET) Godot build.
	return godot::ClassDB::class_exists("CSharpScript");
}

void init() {
	if (s_managed_funcs.init) {
		s_managed_funcs.init();
	}
}

void close() {
	if (s_managed_funcs.close) {
		s_managed_funcs.close();
	}
	s_managed_defined_hooks = DEFINED_NONE;
}

void release_bindings() {
	s_managed_funcs = {};
}

void handle_logger_error(const String &p_file, const String &p_code) {
	if (s_managed_funcs.logger_error) {
		Char16String code_utf16 = p_code.utf16();
		Char16String file_utf16 = p_file.utf16();
		s_managed_funcs.logger_error(
				code_utf16.get_data(), code_utf16.length(),
				file_utf16.get_data(), file_utf16.length());
	}
}

void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	if (s_managed_funcs.add_breadcrumb) {
		Char16String message_utf16 = p_breadcrumb->get_message().utf16();
		Char16String category_utf16 = p_breadcrumb->get_category().utf16();
		Char16String type_utf16 = p_breadcrumb->get_type().utf16();
		// TODO: SentryBreadcrumb::get_data() is not implemented
		s_managed_funcs.add_breadcrumb(
				message_utf16.get_data(), message_utf16.length(),
				category_utf16.get_data(), category_utf16.length(),
				type_utf16.get_data(), type_utf16.length(),
				static_cast<int32_t>(p_breadcrumb->get_level()));
	}
}

void set_tag(const String &p_key, const String &p_value) {
	if (s_managed_funcs.set_tag) {
		Char16String key_utf16 = p_key.utf16();
		Char16String value_utf16 = p_value.utf16();
		s_managed_funcs.set_tag(
				key_utf16.get_data(), key_utf16.length(),
				value_utf16.get_data(), value_utf16.length());
	}
}

void remove_tag(const String &p_key) {
	if (s_managed_funcs.remove_tag) {
		Char16String key_utf16 = p_key.utf16();
		s_managed_funcs.remove_tag(
				key_utf16.get_data(), key_utf16.length());
	}
}

void set_user(const Ref<SentryUser> &p_user) {
	if (s_managed_funcs.set_user) {
		Char16String id_utf16 = p_user->get_id().utf16();
		Char16String username_utf16 = p_user->get_username().utf16();
		Char16String email_utf16 = p_user->get_email().utf16();
		Char16String ip_utf16 = p_user->get_ip_address().utf16();

		s_managed_funcs.set_user(
				id_utf16.get_data(), id_utf16.length(),
				username_utf16.get_data(), username_utf16.length(),
				email_utf16.get_data(), email_utf16.length(),
				ip_utf16.get_data(), ip_utf16.length());
	}
}

void remove_user() {
	if (s_managed_funcs.remove_user) {
		s_managed_funcs.remove_user();
	}
}

bool process_event_in_managed_layer(const Ref<SentryEvent> &p_event) {
	FAIL_COND_V_PRINT_ERROR(p_event.is_null(), true, "Internal error: options.Native.SetBeforeSend received a null native event.");

	if (s_managed_funcs.process_native_event == nullptr || !s_managed_defined_hooks.has_flag(DEFINED_BEFORE_SEND)) {
		// .NET layer unavailable, or no before-send callback registered.
		return true;
	}

	static thread_local bool in_before_send = false;
	if (in_before_send) {
		return true;
	}
	in_before_send = true;

	const bool keep = s_managed_funcs.process_native_event((void *)p_event.ptr()) != 0;

	in_before_send = false;
	return keep;
}

bool is_managed_layer_registered() {
	return s_managed_funcs.init != nullptr;
}

#ifdef TESTS_ENABLED

bool is_before_send_defined() {
	return s_managed_defined_hooks.has_flag(DEFINED_BEFORE_SEND);
}

#endif // TESTS_ENABLED

} // namespace sentry::dotnet
