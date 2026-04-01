#include "gen/sdk_version.gen.h"
#include "sentry/environment.h"
#include "sentry/sentry_sdk.h"

#ifdef _WIN32
#define CSHARP_EXPORT __declspec(dllexport)
#else
#define CSHARP_EXPORT __attribute__((visibility("default")))
#endif

using namespace sentry;

static void (*s_dotnet_init_fn)() = nullptr;

extern "C" {

// Native-owned string handle for passing Godot Strings across the interop boundary.
// C# must call csharp_interop_string_free() after reading the data.
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

struct OptionsData {
	// NOTE: Strings are native-owned, but C# should "take" all strings or it will leak.
	GodotStringHandle dsn;
	GodotStringHandle release;
	GodotStringHandle dist;
	GodotStringHandle environment;

	// Value types
	bool debug;
	int32_t diagnostic_level;
	double sample_rate;
	int32_t max_breadcrumbs;
	double shutdown_timeout_ms;
	bool send_default_pii;
	bool enable_logs;

	// Godot-specific
	bool attach_log;
	bool attach_scene_tree;
	bool attach_screenshot;
	int32_t screenshot_level;
	bool app_hang_tracking;
	double app_hang_timeout_sec;

	// Logger
	bool logger_enabled;
	bool logger_include_source;
	bool logger_include_variables;
	bool logger_messages_as_breadcrumbs;
	int32_t logger_event_mask;
	int32_t logger_breadcrumb_mask;

	// Experimental
	bool enable_metrics;
};

void _populate_options_data(OptionsData &r_data, const Ref<SentryOptions> &options) {
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
	r_data.logger_messages_as_breadcrumbs = options->is_logger_messages_as_breadcrumbs_enabled();
	r_data.logger_event_mask = options->get_logger_event_mask();
	r_data.logger_breadcrumb_mask = options->get_logger_breadcrumb_mask();
	r_data.enable_metrics = options->get_experimental()->get_enable_metrics();
}

// *** Functions called from C#

CSHARP_EXPORT OptionsData csharp_interop_get_options() {
	OptionsData data;
	_populate_options_data(data, SentrySDK::get_singleton()->get_options());
	return data;
}

CSHARP_EXPORT OptionsData csharp_interop_get_options_defaults() {
	OptionsData data;
	_populate_options_data(data, SentryOptions::create_from_project_settings());
	return data;
}

CSHARP_EXPORT GodotStringHandle csharp_interop_detect_environment() {
	return _make_handle(environment::detect_godot_environment());
}

CSHARP_EXPORT bool csharp_interop_sdk_is_enabled() {
	return SentrySDK::get_singleton()->is_enabled();
}

CSHARP_EXPORT void csharp_interop_sdk_set_tag(const char16_t *key, int key_len, const char16_t *value, int value_len) {
	String k = String::utf16(key, key_len);
	String v = String::utf16(value, value_len);
	SentrySDK::get_singleton()->set_tag(k, v);
}

CSHARP_EXPORT void csharp_interop_sdk_remove_tag(const char16_t *key, int key_len) {
	String k = String::utf16(key, key_len);
	SentrySDK::get_singleton()->remove_tag(k);
}

CSHARP_EXPORT void csharp_interop_set_trace(const char *p_trace_id, const char *p_parent_span_id) {
	SentrySDK::get_singleton()->set_trace(p_trace_id, p_parent_span_id);
}

CSHARP_EXPORT const char *csharp_interop_get_sdk_version() {
	return SENTRY_GODOT_SDK_VERSION;
}

CSHARP_EXPORT void csharp_interop_register_dotnet_init(void (*fn)()) {
	s_dotnet_init_fn = fn;
}

} // extern "C"

// *** Functions called from native

namespace sentry::dotnet {

void init() {
	if (s_dotnet_init_fn) {
		s_dotnet_init_fn();
	}
}

} // namespace sentry::dotnet
