#include "gen/sdk_version.gen.h"
#include "sentry/dotnet/dotnet_scope_observer.h"
#include "sentry/dotnet/process_default_attachments.h"
#include "sentry/environment.h"
#include "sentry/logging/print.h"
#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_sdk.h"
#include "sentry/sentry_user.h"

#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/project_settings.hpp>

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
	void (*logger_error)(const char16_t *code, int32_t code_len, const char16_t *file, int32_t file_len);
	void (*add_breadcrumb)(const char16_t *message, int32_t message_len, const char16_t *category, int32_t category_len, const char16_t *type, int32_t type_len, int32_t level);
	void (*set_tag)(const char16_t *name, int32_t name_len, const char16_t *value, int32_t value_len);
	void (*remove_tag)(const char16_t *name, int32_t name_len);
	void (*set_user)(const char16_t *id, int32_t id_len, const char16_t *username, int32_t username_len, const char16_t *email, int32_t email_len, const char16_t *ip, int32_t ip_len);
	void (*remove_user)();
};

static ManagedFunctions s_managed_funcs = {};

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
	uint8_t app_hang_tracking;
	double app_hang_timeout_sec;

	// Logger
	uint8_t logger_enabled;
	uint8_t logger_include_source;
	uint8_t logger_include_variables;
	int32_t logger_event_mask;
	int32_t logger_breadcrumb_mask;
	int32_t logger_log_mask;

	// Experimental
	uint8_t enable_metrics;
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
	uint8_t app_hang_tracking;
	double app_hang_timeout_sec;

	uint8_t logger_enabled;
	uint8_t logger_include_source;
	uint8_t logger_include_variables;
	int32_t logger_event_mask;
	int32_t logger_breadcrumb_mask;
	int32_t logger_log_mask;

	uint8_t enable_metrics;
};

struct NativeTraceContext {
	GodotStringHandle trace_id;
	GodotStringHandle parent_span_id;
};

static void _apply_managed_options(const ManagedOptions &data, Ref<SentryOptions> options) {
	Ref<SentryLoggerLimits> logger_limits = options->get_logger_limits();
	if (logger_limits.is_null()) {
		logger_limits.instantiate();
		options->set_logger_limits(logger_limits);
	}
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
	options->set_app_hang_tracking(data.app_hang_tracking);
	options->set_app_hang_timeout_sec(data.app_hang_timeout_sec);
	options->set_logger_enabled(data.logger_enabled);
	options->set_logger_include_source(data.logger_include_source);
	options->set_logger_include_variables(data.logger_include_variables);
	options->set_logger_event_mask(data.logger_event_mask);
	options->set_logger_breadcrumb_mask(data.logger_breadcrumb_mask);
	options->set_logger_log_mask(data.logger_log_mask);
	options->get_experimental()->set_enable_metrics(data.enable_metrics);
}

void _populate_options_data(NativeOptions &r_data, const Ref<SentryOptions> &options) {
	r_data = {};

	r_data.logger_limits.events_per_frame = options->get_logger_limits()->get_events_per_frame();
	r_data.logger_limits.repeated_error_window_ms = options->get_logger_limits()->get_repeated_error_window_ms();
	r_data.logger_limits.throttle_events = options->get_logger_limits()->get_throttle_events();
	r_data.logger_limits.throttle_window_ms = options->get_logger_limits()->get_throttle_window_ms();

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
	r_data.app_hang_tracking = options->is_app_hang_tracking_enabled();
	r_data.app_hang_timeout_sec = options->get_app_hang_timeout_sec();
	r_data.logger_enabled = options->is_logger_enabled();
	r_data.logger_include_source = options->is_logger_include_source_enabled();
	r_data.logger_include_variables = options->is_logger_include_variables_enabled();
	r_data.logger_event_mask = options->get_logger_event_mask();
	r_data.logger_breadcrumb_mask = options->get_logger_breadcrumb_mask();
	r_data.logger_log_mask = options->get_logger_log_mask();
	r_data.enable_metrics = options->get_experimental()->get_enable_metrics();
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
			items[i].path = _make_handle(atts[i]->get_path());
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

static ManagedOptions s_pending_managed_opts;

static void _apply_pending_managed_options(const Ref<SentryOptions> &options) {
	_apply_managed_options(s_pending_managed_opts, options);
}

CSHARP_EXPORT void csharp_interop_sdk_init(ManagedOptions managed_opts) {
	s_pending_managed_opts = managed_opts;
	SentrySDK::get_singleton()->init(callable_mp_static(_apply_pending_managed_options));
	s_pending_managed_opts = {};
}

CSHARP_EXPORT void csharp_interop_sdk_close() {
	SentrySDK::get_singleton()->close();
}

CSHARP_EXPORT void csharp_interop_process_default_attachments() {
	sentry::dotnet::process_default_attachments();
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

CSHARP_EXPORT const char *csharp_interop_get_sdk_version() {
	return SENTRY_GODOT_SDK_VERSION;
}

CSHARP_EXPORT void csharp_interop_log(int32_t level, const char16_t *msg, int32_t len) {
	sentry::logging::print(
			static_cast<sentry::Level>(level),
			String::utf16(msg, len));
}

} // extern "C"

// *** Functions called from native

namespace sentry::dotnet {

void init() {
	if (s_managed_funcs.init) {
		s_managed_funcs.init();
	}
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

} // namespace sentry::dotnet
