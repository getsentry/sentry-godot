#include "gen/sdk_version.gen.h"
#include "sentry/environment.h"
#include "sentry/logging/print.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/engine_debugger.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#ifdef _WIN32
#define CSHARP_EXPORT __declspec(dllexport)
#else
#define CSHARP_EXPORT __attribute__((visibility("default")))
#endif

using namespace sentry;

static void (*s_dotnet_init_fn)() = nullptr;
static void (*s_dotnet_logger_error_fn)(const char16_t *code, int32_t code_len, const char16_t *file, int32_t file_len) = nullptr;

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

// Must match layout of LoggerLimitsData in NativeBridge.cs.
struct LoggerLimitsData {
	int32_t events_per_frame;
	int32_t repeated_error_window_ms;
	int32_t throttle_events;
	int32_t throttle_window_ms;
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
	uint8_t logger_messages_as_breadcrumbs;
	int32_t logger_event_mask;
	int32_t logger_breadcrumb_mask;

	// Experimental
	uint8_t enable_metrics;
};

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
	uint8_t logger_messages_as_breadcrumbs;
	int32_t logger_event_mask;
	int32_t logger_breadcrumb_mask;

	uint8_t enable_metrics;
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
	options->set_logger_messages_as_breadcrumbs(data.logger_messages_as_breadcrumbs);
	options->set_logger_event_mask(data.logger_event_mask);
	options->set_logger_breadcrumb_mask(data.logger_breadcrumb_mask);
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
	r_data.logger_messages_as_breadcrumbs = options->is_logger_messages_as_breadcrumbs_enabled();
	r_data.logger_event_mask = options->get_logger_event_mask();
	r_data.logger_breadcrumb_mask = options->get_logger_breadcrumb_mask();
	r_data.enable_metrics = options->get_experimental()->get_enable_metrics();
}

// *** Functions called from C#

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

CSHARP_EXPORT void csharp_interop_register_dotnet_init(void (*fn)()) {
	s_dotnet_init_fn = fn;
}

CSHARP_EXPORT void csharp_interop_register_logger_error_handler(
		void (*fn)(const char16_t *code, int32_t code_len, const char16_t *file, int32_t file_len)) {
	s_dotnet_logger_error_fn = fn;
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
	if (s_dotnet_init_fn) {
		s_dotnet_init_fn();
	}
}

void handle_logger_error(const String &p_file, const String &p_code) {
	if (s_dotnet_logger_error_fn) {
		Char16String code_utf16 = p_code.utf16();
		Char16String file_utf16 = p_file.utf16();
		s_dotnet_logger_error_fn(
				(const char16_t *)code_utf16.get_data(), code_utf16.length(),
				(const char16_t *)file_utf16.get_data(), file_utf16.length());
	}
}

} // namespace sentry::dotnet
