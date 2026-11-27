#include "sentry_options.h"

#include "sentry/environment.h"
#include "sentry/settings_migrations.h"
#include "sentry_sdk.h" // for SentrySDK::Level variant casts

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/core/math.hpp>

namespace {

uint64_t normalize_metrics_interval_usec(double p_interval_sec) {
	if (!Math::is_finite(p_interval_sec)) {
		return 1'000'000;
	}
	constexpr double one_day = 86'400.0;
	p_interval_sec = CLAMP(p_interval_sec, 1.0, one_day);
	return static_cast<uint64_t>(p_interval_sec * 1'000'000.0);
}

void _define_setting(const String &p_setting, const Variant &p_default, bool p_basic = true) {
	if (!ProjectSettings::get_singleton()->has_setting(p_setting)) {
		ProjectSettings::get_singleton()->set(p_setting, p_default);
	}
	ProjectSettings::get_singleton()->set_initial_value(p_setting, p_default);
	ProjectSettings::get_singleton()->set_as_basic(p_setting, p_basic);
	ProjectSettings::get_singleton()->set_restart_if_changed(p_setting, false);

	// Preserve order of the defined settings.
	static int32_t config_value_order = 0;
	if (config_value_order == 0) {
		config_value_order = ProjectSettings::get_singleton()->get_order(p_setting);
	}
	ProjectSettings::get_singleton()->set_order(p_setting, config_value_order);
	config_value_order++;
}

inline void _requires_restart(const String &p_setting) {
	ProjectSettings::get_singleton()->set_restart_if_changed(p_setting, true);
}

void _define_setting(const godot::PropertyInfo &p_info, const godot::Variant &p_default, bool p_basic = true) {
	_define_setting(p_info.name, p_default, p_basic);
	Dictionary info = (Dictionary)p_info;
	info.erase("usage"); // Fix "usage" not supported warning.
	ProjectSettings::get_singleton()->add_property_info(info);
}

} // unnamed namespace

namespace sentry {

// *** SentryLoggerLimits

void SentryLoggerLimits::_bind_methods() {
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, events_per_frame);
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, repeated_error_window_ms);
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, throttle_events);
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, throttle_window_ms);
}

// *** SentryGodotLoggerOptions

void SentryGodotLoggerOptions::deprecated_set_limits(const Ref<SentryLoggerLimits> &p_limits) {
	limits = p_limits;
	if (limits.is_null()) {
		limits.instantiate();
	}
}

void SentryGodotLoggerOptions::_bind_methods() {
	BIND_PROPERTY_SIMPLE(SentryGodotLoggerOptions, Variant::BOOL, enabled);
	BIND_PROPERTY_SIMPLE(SentryGodotLoggerOptions, Variant::BOOL, include_source_context);
	BIND_PROPERTY_SIMPLE(SentryGodotLoggerOptions, Variant::BOOL, include_variables);
	BIND_PROPERTY_SIMPLE(SentryGodotLoggerOptions, Variant::INT, event_mask);
	BIND_PROPERTY_SIMPLE(SentryGodotLoggerOptions, Variant::INT, breadcrumb_mask);
	BIND_PROPERTY_SIMPLE(SentryGodotLoggerOptions, Variant::INT, log_mask);

	BIND_PROPERTY_READONLY(SentryGodotLoggerOptions, PropertyInfo(Variant::OBJECT, "limits", PROPERTY_HINT_TYPE_STRING, "SentryLoggerLimits", PROPERTY_USAGE_NONE), get_limits);
}

SentryGodotLoggerOptions::SentryGodotLoggerOptions() {
	limits.instantiate();
}

// *** SentryExperimental

void SentryExperimental::deprecated_set_enable_metrics(bool) {
	WARN_DEPRECATED_MSG("The \"enable_metrics\" option is deprecated, has no effect, and will be removed in version 3.0. Metrics are sent when you use SentrySDK.metrics.");
}

bool SentryExperimental::deprecated_get_enable_metrics() {
	return true;
}

void SentryExperimental::deprecated_set_before_send_metric(Callable p_value) {
	WARN_DEPRECATED_MSG("Sentry Metrics are now generally available. This property is deprecated. Use SentryOptions.before_send_metric instead.");
	ERR_FAIL_NULL(owner);
	owner->set_before_send_metric(p_value);
}

Callable SentryExperimental::deprecated_get_before_send_metric() {
	return owner ? owner->get_before_send_metric() : Callable();
}

void SentryExperimental::_bind_methods() {
	// DEPRECATED: These properties are deprecated and remain for compatibility reasons.
	// TODO: Remove these after December 2026 or in version 3.0.
	BIND_PROPERTY_SIMPLE_DEPRECATED(SentryExperimental, Variant::BOOL, enable_metrics);
	BIND_PROPERTY_SIMPLE_DEPRECATED(SentryExperimental, Variant::CALLABLE, before_send_metric);
}

void SentryNativeOptions::_bind_methods() {
	BIND_ENUM_CONSTANT(CRASH_REPORTING_MODE_MINIDUMP);
	BIND_ENUM_CONSTANT(CRASH_REPORTING_MODE_NATIVE);
	BIND_ENUM_CONSTANT(CRASH_REPORTING_MODE_NATIVE_WITH_MINIDUMP);
	BIND_PROPERTY(SentryNativeOptions, PropertyInfo(Variant::INT, "crash_reporting_mode", PROPERTY_HINT_ENUM, "Minidump Only:0,Native Stackwalking Only:1,Native Stackwalking With Minidump:2"), set_crash_reporting_mode, get_crash_reporting_mode);

	BIND_ENUM_CONSTANT(MINIDUMP_MODE_STACK_ONLY);
	BIND_ENUM_CONSTANT(MINIDUMP_MODE_SMART);
	BIND_ENUM_CONSTANT(MINIDUMP_MODE_FULL);
	BIND_PROPERTY(SentryNativeOptions, PropertyInfo(Variant::INT, "minidump_mode", PROPERTY_HINT_ENUM, "Stack Only:0,Smart:1,Full Memory:2"), set_minidump_mode, get_minidump_mode);

	BIND_ENUM_CONSTANT(THREAD_STACKWALK_MODE_CRASHED_ONLY);
	BIND_ENUM_CONSTANT(THREAD_STACKWALK_MODE_ALL);
	BIND_PROPERTY(SentryNativeOptions, PropertyInfo(Variant::INT, "thread_stackwalk_mode", PROPERTY_HINT_ENUM, "Crashed Thread Only:1,All Threads:2"), set_thread_stackwalk_mode, get_thread_stackwalk_mode);

	BIND_ENUM_CONSTANT(CRASH_UPLOAD_MODE_SYNC);
	BIND_ENUM_CONSTANT(CRASH_UPLOAD_MODE_ASYNC);
	BIND_PROPERTY(SentryNativeOptions, PropertyInfo(Variant::INT, "crash_upload_mode", PROPERTY_HINT_ENUM, "Synchronous:0,Asynchronous:1"), set_crash_upload_mode, get_crash_upload_mode);
}

// *** SentryCrashReporterOptions

void SentryCrashReporterOptions::_bind_methods() {
	BIND_ENUM_CONSTANT(VIEW_FEEDBACK);
	BIND_ENUM_CONSTANT(VIEW_TAGS);
	BIND_ENUM_CONSTANT(VIEW_CONTEXTS);
	BIND_ENUM_CONSTANT(VIEW_EXTRA);
	BIND_ENUM_CONSTANT(VIEW_SDK);
	BIND_ENUM_CONSTANT(VIEW_USER);
	BIND_ENUM_CONSTANT(VIEW_ATTACHMENTS);
	BIND_ENUM_CONSTANT(VIEW_STACKTRACE);
	BIND_ENUM_CONSTANT(VIEW_ENVELOPE);

	BIND_PROPERTY_SIMPLE(SentryCrashReporterOptions, Variant::BOOL, enabled);
	BIND_PROPERTY_SIMPLE(SentryCrashReporterOptions, Variant::STRING, path);
	BIND_PROPERTY_SIMPLE(SentryCrashReporterOptions, Variant::STRING, window_title);
	BIND_PROPERTY_SIMPLE(SentryCrashReporterOptions, Variant::STRING, header_text);
	BIND_PROPERTY_SIMPLE(SentryCrashReporterOptions, Variant::STRING, header_description);
	BIND_PROPERTY_SIMPLE(SentryCrashReporterOptions, Variant::STRING, submit_button_label);
	BIND_PROPERTY_SIMPLE(SentryCrashReporterOptions, Variant::STRING, cancel_button_label);
	BIND_PROPERTY_SIMPLE(SentryCrashReporterOptions, Variant::COLOR, accent_color);
	BIND_PROPERTY(SentryCrashReporterOptions, PropertyInfo(Variant::STRING, "logo_light_theme", PROPERTY_HINT_FILE, "*.png"), set_logo_light_theme, get_logo_light_theme);
	BIND_PROPERTY(SentryCrashReporterOptions, PropertyInfo(Variant::STRING, "logo_dark_theme", PROPERTY_HINT_FILE, "*.png"), set_logo_dark_theme, get_logo_dark_theme);
	BIND_PROPERTY_SIMPLE(SentryCrashReporterOptions, Variant::BOOL, window_closable);
	BIND_PROPERTY(SentryCrashReporterOptions, PropertyInfo(Variant::ARRAY, "views", PROPERTY_HINT_TYPE_STRING, get_view_hint_string()), set_views, get_views);
}

String SentryCrashReporterOptions::get_path_setting(const String &p_platform, const String &p_arch) {
	return vformat("sentry/crash_reporter/path_%s_%s", p_platform, p_arch);
}

String SentryCrashReporterOptions::get_default_path(const String &p_platform, const String &p_arch) {
	// No x86_32 build of the Sentry reporter exists.
	if (p_platform == "windows" && (p_arch == "x86_64" || p_arch == "arm64")) {
		return vformat("res://addons/sentry/crash_reporter/windows/%s/Sentry.CrashReporter.exe", p_arch);
	}
	if (p_platform == "linux" && (p_arch == "x86_64" || p_arch == "arm64")) {
		return vformat("res://addons/sentry/crash_reporter/linux/%s/Sentry.CrashReporter", p_arch);
	}
	return "";
}

String SentryCrashReporterOptions::get_current_path_setting() {
#if defined(WINDOWS_ENABLED)
	return get_path_setting("windows", Engine::get_singleton()->get_architecture_name());
#elif defined(LINUX_ENABLED)
	return get_path_setting("linux", Engine::get_singleton()->get_architecture_name());
#else
	return "";
#endif
}

String SentryCrashReporterOptions::get_view_hint_string() {
	return vformat("%d/%d:Feedback:0,Tags:1,Contexts:2,Additional Data:3,SDK:4,User:5,Attachments:6,Stacktrace:7,Envelope:8",
			Variant::INT, PROPERTY_HINT_ENUM);
}

// *** SentryAndroidOptions

void SentryAndroidOptions::_bind_methods() {
	BIND_PROPERTY_SIMPLE(SentryAndroidOptions, Variant::BOOL, enable_anr_detection);
	BIND_PROPERTY_SIMPLE(SentryAndroidOptions, Variant::INT, anr_timeout_interval_ms);
	BIND_PROPERTY_SIMPLE(SentryAndroidOptions, Variant::BOOL, attach_anr_thread_dump);
}

// *** SentryAppleOptions

void SentryAppleOptions::_bind_methods() {
	BIND_PROPERTY_SIMPLE(SentryAppleOptions, Variant::BOOL, enable_metrickit);
}

// *** SentryWebOptions

void SentryWebOptions::_bind_methods() {
	BIND_PROPERTY_SIMPLE(SentryWebOptions, Variant::BOOL, enable_tunnel);
	BIND_PROPERTY_SIMPLE(SentryWebOptions, Variant::STRING, tunnel_path);
	BIND_PROPERTY_SIMPLE(SentryWebOptions, Variant::STRING, tunnel_base_path);
	BIND_PROPERTY_SIMPLE(SentryWebOptions, Variant::BOOL, tunnel_trailing_slash);
}

// *** SentryAutoMetricsOptions

uint64_t SentryAutoMetricsOptions::get_normalized_frame_metrics_interval_usec() const {
	return normalize_metrics_interval_usec(frame_metrics_interval_sec);
}

uint64_t SentryAutoMetricsOptions::get_normalized_rendering_metrics_interval_usec() const {
	return normalize_metrics_interval_usec(rendering_metrics_interval_sec);
}

uint64_t SentryAutoMetricsOptions::get_normalized_memory_metrics_interval_usec() const {
	return normalize_metrics_interval_usec(memory_metrics_interval_sec);
}

uint64_t SentryAutoMetricsOptions::get_normalized_network_metrics_interval_usec() const {
	return normalize_metrics_interval_usec(network_metrics_interval_sec);
}

void SentryAutoMetricsOptions::_bind_methods() {
	BIND_PROPERTY_SIMPLE(SentryAutoMetricsOptions, Variant::BOOL, enable_frame_metrics);
	BIND_PROPERTY_SIMPLE(SentryAutoMetricsOptions, Variant::FLOAT, frame_metrics_interval_sec);
	BIND_PROPERTY_SIMPLE(SentryAutoMetricsOptions, Variant::BOOL, enable_rendering_metrics);
	BIND_PROPERTY_SIMPLE(SentryAutoMetricsOptions, Variant::FLOAT, rendering_metrics_interval_sec);
	BIND_PROPERTY_SIMPLE(SentryAutoMetricsOptions, Variant::BOOL, enable_memory_metrics);
	BIND_PROPERTY_SIMPLE(SentryAutoMetricsOptions, Variant::FLOAT, memory_metrics_interval_sec);
	BIND_PROPERTY_SIMPLE(SentryAutoMetricsOptions, Variant::BOOL, enable_network_metrics);
	BIND_PROPERTY_SIMPLE(SentryAutoMetricsOptions, Variant::FLOAT, network_metrics_interval_sec);
}

// *** SentryOptions

void SentryOptions::_define_project_settings(const Ref<SentryOptions> &p_options) {
	ERR_FAIL_COND(p_options.is_null());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	sentry::run_project_settings_migrations();

	_define_setting("sentry/options/auto_init", p_options->auto_init);
	_define_setting("sentry/options/skip_auto_init_on_editor_play", p_options->skip_auto_init_on_editor_play);

	_define_setting("sentry/options/dsn", p_options->dsn);
	_define_setting("sentry/options/release", p_options->release, false);
	_define_setting("sentry/options/dist", p_options->dist, false);
	_define_setting("sentry/options/environment", p_options->environment, false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/debug_printing", PROPERTY_HINT_ENUM, "Off,On,Auto"), (int)SentryOptions::DEBUG_DEFAULT);
	_define_setting(sentry::make_level_enum_property("sentry/options/diagnostic_level"), p_options->diagnostic_level);
	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/options/sample_rate", PROPERTY_HINT_RANGE, "0.0,1.0"), p_options->sample_rate, false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/max_breadcrumbs", PROPERTY_HINT_RANGE, "0, 500"), p_options->max_breadcrumbs, false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/shutdown_timeout_ms", PROPERTY_HINT_RANGE, "0,30000"), p_options->shutdown_timeout_ms, false);
	_define_setting("sentry/options/send_default_pii", p_options->send_default_pii);
	_define_setting("sentry/options/auto_session_tracking", p_options->auto_session_tracking, false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/session_tracking_interval_ms", PROPERTY_HINT_RANGE, "0,120000,1,or_greater"), p_options->session_tracking_interval_ms, false);

	_define_setting("sentry/options/attach_log", p_options->attach_log, false);
	_define_setting("sentry/options/attach_scene_tree", p_options->attach_scene_tree);

	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/options/tracing/traces_sample_rate", PROPERTY_HINT_RANGE, "0.0,1.0"), p_options->traces_sample_rate, false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/tracing/trace_lifecycle", PROPERTY_HINT_ENUM, "Static,Stream"), p_options->trace_lifecycle, false);
	_define_setting("sentry/options/tracing/trace_propagation_targets", PackedStringArray(p_options->trace_propagation_targets), false);
	_define_setting("sentry/options/tracing/propagate_traceparent", p_options->propagate_traceparent, false);
	_define_setting("sentry/options/tracing/org_id", p_options->org_id, false);

	_define_setting("sentry/options/app_hang/tracking", p_options->enable_app_hang_tracking, false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/app_hang/timeout_ms", PROPERTY_HINT_RANGE, "1000,10000,1"), p_options->app_hang_timeout_ms, false);

	Ref<SentryAutoMetricsOptions> auto_metrics = p_options->get_auto_metrics();
	_define_setting("sentry/auto_metrics/frame_metrics/enabled", auto_metrics->get_enable_frame_metrics());
	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/auto_metrics/frame_metrics/interval_sec", PROPERTY_HINT_RANGE, "1,60,0.1,or_greater"), auto_metrics->get_frame_metrics_interval_sec());
	_define_setting("sentry/auto_metrics/rendering_metrics/enabled", auto_metrics->get_enable_rendering_metrics());
	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/auto_metrics/rendering_metrics/interval_sec", PROPERTY_HINT_RANGE, "1,60,0.1,or_greater"), auto_metrics->get_rendering_metrics_interval_sec());
	_define_setting("sentry/auto_metrics/memory_metrics/enabled", auto_metrics->get_enable_memory_metrics());
	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/auto_metrics/memory_metrics/interval_sec", PROPERTY_HINT_RANGE, "1,300,0.1,or_greater"), auto_metrics->get_memory_metrics_interval_sec());
	_define_setting("sentry/auto_metrics/network_metrics/enabled", auto_metrics->get_enable_network_metrics());
	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/auto_metrics/network_metrics/interval_sec", PROPERTY_HINT_RANGE, "1,60,0.1,or_greater"), auto_metrics->get_network_metrics_interval_sec());

	Ref<SentryGodotLoggerOptions> logger_options = p_options->get_godot_logger();
	_define_setting("sentry/godot_logger/enabled", logger_options->get_enabled());
	_define_setting("sentry/godot_logger/include_source_context", logger_options->get_include_source_context(), false);
	_define_setting("sentry/godot_logger/include_variables", logger_options->get_include_variables(), false);
	_requires_restart("sentry/godot_logger/include_variables");
	_define_setting(PropertyInfo(Variant::INT, "sentry/godot_logger/events", PROPERTY_HINT_FLAGS, sentry::GODOT_ERROR_MASK_EXPORT_STRING_FOR_EVENTS()), logger_options->get_event_mask(), false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/godot_logger/breadcrumbs", PROPERTY_HINT_FLAGS, sentry::GODOT_ERROR_MASK_EXPORT_STRING()), logger_options->get_breadcrumb_mask(), false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/godot_logger/logs", PROPERTY_HINT_FLAGS, sentry::GODOT_ERROR_MASK_EXPORT_STRING()), logger_options->get_log_mask(), false);

	Ref<SentryLoggerLimits> limits = logger_options->get_limits();
	_define_setting(PropertyInfo(Variant::INT, "sentry/godot_logger/limits/events_per_frame", PROPERTY_HINT_RANGE, "0,20"), limits->get_events_per_frame(), false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/godot_logger/limits/repeated_error_window_ms", PROPERTY_HINT_RANGE, "0,10000"), limits->get_repeated_error_window_ms(), false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/godot_logger/limits/throttle_events", PROPERTY_HINT_RANGE, "0,20"), limits->get_throttle_events(), false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/godot_logger/limits/throttle_window_ms", PROPERTY_HINT_RANGE, "0,10000"), limits->get_throttle_window_ms(), false);

	Ref<SentryNativeOptions> native = p_options->get_native();
	_define_setting(PropertyInfo(Variant::INT, "sentry/native/native_crash_backend/crash_reporting_mode", PROPERTY_HINT_ENUM, "Minidump Only:0,Native Stackwalking Only:1,Native Stackwalking With Minidump:2"), native->get_crash_reporting_mode(), false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/native/native_crash_backend/minidump_mode", PROPERTY_HINT_ENUM, "Stack Only:0,Smart:1,Full Memory:2"), native->get_minidump_mode(), false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/native/native_crash_backend/thread_stackwalk_mode", PROPERTY_HINT_ENUM, "Crashed Thread Only:1,All Threads:2"), native->get_thread_stackwalk_mode(), false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/native/native_crash_backend/crash_upload_mode", PROPERTY_HINT_ENUM, "Synchronous:0,Asynchronous:1"), native->get_crash_upload_mode(), false);

	Ref<SentryCrashReporterOptions> crash_reporter = p_options->get_crash_reporter();
	_define_setting("sentry/crash_reporter/enabled", crash_reporter->get_enabled());
	for (const char *platform : { "windows", "linux" }) {
		for (const char *arch : { "x86_64", "arm64", "x86_32" }) {
			_define_setting(PropertyInfo(Variant::STRING, SentryCrashReporterOptions::get_path_setting(platform, arch), PROPERTY_HINT_FILE),
					SentryCrashReporterOptions::get_default_path(platform, arch), false);
		}
	}
	_define_setting("sentry/crash_reporter/appearance/window_title", crash_reporter->get_window_title(), false);
	_define_setting("sentry/crash_reporter/appearance/header_text", crash_reporter->get_header_text(), false);
	_define_setting(PropertyInfo(Variant::STRING, "sentry/crash_reporter/appearance/header_description", PROPERTY_HINT_MULTILINE_TEXT), crash_reporter->get_header_description(), false);
	_define_setting("sentry/crash_reporter/appearance/submit_button_label", crash_reporter->get_submit_button_label(), false);
	_define_setting("sentry/crash_reporter/appearance/cancel_button_label", crash_reporter->get_cancel_button_label(), false);
	_define_setting(PropertyInfo(Variant::COLOR, "sentry/crash_reporter/appearance/accent_color"), crash_reporter->get_accent_color(), false);
	_define_setting(PropertyInfo(Variant::STRING, "sentry/crash_reporter/appearance/logo_light_theme", PROPERTY_HINT_FILE, "*.png"), crash_reporter->get_logo_light_theme(), false);
	_define_setting(PropertyInfo(Variant::STRING, "sentry/crash_reporter/appearance/logo_dark_theme", PROPERTY_HINT_FILE, "*.png"), crash_reporter->get_logo_dark_theme(), false);
	_define_setting("sentry/crash_reporter/appearance/window_closable", crash_reporter->get_window_closable(), false);
	_define_setting(PropertyInfo(Variant::ARRAY, "sentry/crash_reporter/appearance/views", PROPERTY_HINT_TYPE_STRING, SentryCrashReporterOptions::get_view_hint_string()), crash_reporter->get_views(), false);

	_define_setting(PropertyInfo(Variant::BOOL, "sentry/android/application_not_responding/enable_detection"), p_options->get_android()->get_enable_anr_detection(), false);
	_define_setting(PropertyInfo(Variant::INT, "sentry/android/application_not_responding/timeout_interval_ms"), p_options->get_android()->get_anr_timeout_interval_ms(), false);
	_define_setting(PropertyInfo(Variant::BOOL, "sentry/android/application_not_responding/attach_thread_dump"), p_options->get_android()->get_attach_anr_thread_dump(), false);

	_define_setting(PropertyInfo(Variant::BOOL, "sentry/apple/metrickit/enabled"), p_options->get_apple()->get_enable_metrickit(), false);

	Ref<SentryWebOptions> web = p_options->get_web();
	_define_setting("sentry/web/tunnel/enabled", web->get_enable_tunnel());
	_define_setting("sentry/web/tunnel/path", web->get_tunnel_path());
	_define_setting("sentry/web/tunnel/base_path", web->get_tunnel_base_path());
	_define_setting("sentry/web/tunnel/trailing_slash", web->get_tunnel_trailing_slash());

	_define_setting("sentry/experimental/attach_screenshot", p_options->attach_screenshot);
	_define_setting(sentry::make_level_enum_property("sentry/experimental/screenshot_level"), p_options->screenshot_level, false);
}

void SentryOptions::_load_project_settings(const Ref<SentryOptions> &p_options) {
	ERR_FAIL_COND(p_options.is_null());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());
	ERR_FAIL_NULL(OS::get_singleton());

	p_options->auto_init = ProjectSettings::get_singleton()->get_setting("sentry/options/auto_init", p_options->auto_init);
	p_options->skip_auto_init_on_editor_play = ProjectSettings::get_singleton()->get_setting("sentry/options/skip_auto_init_on_editor_play", p_options->skip_auto_init_on_editor_play);

	// DSN: project setting > env var > default.
	String ps_dsn = ProjectSettings::get_singleton()->get_setting("sentry/options/dsn", p_options->dsn);
	if (ps_dsn != p_options->dsn) {
		p_options->set_dsn(ps_dsn);
	} else {
		String env_dsn = OS::get_singleton()->get_environment("SENTRY_DSN");
		if (!env_dsn.is_empty()) {
			p_options->set_dsn(env_dsn);
		}
	}

	// Release: project setting > env var > default.
	String ps_release = ProjectSettings::get_singleton()->get_setting("sentry/options/release", p_options->release);
	if (ps_release != p_options->release) {
		p_options->set_release(ps_release);
	} else {
		String env_release = OS::get_singleton()->get_environment("SENTRY_RELEASE");
		if (!env_release.is_empty()) {
			p_options->set_release(env_release);
		} else {
			// Apply format substitution on the default value.
			p_options->set_release(p_options->release);
		}
	}

	p_options->dist = ProjectSettings::get_singleton()->get_setting("sentry/options/dist", p_options->dist);

	// Environment: project setting > env var > default.
	String ps_environment = ProjectSettings::get_singleton()->get_setting("sentry/options/environment", p_options->environment);
	if (ps_environment != p_options->environment) {
		p_options->set_environment(ps_environment);
	} else {
		String env_environment = OS::get_singleton()->get_environment("SENTRY_ENVIRONMENT");
		if (!env_environment.is_empty()) {
			p_options->set_environment(env_environment);
		} else {
			// Apply format substitution on the default value.
			p_options->set_environment(p_options->environment);
		}
	}

	// DebugMode is only used to represent the debug option in the project settings.
	// The user may also set the `debug` option explicitly in a configuration callback.
	DebugMode mode = (DebugMode)(int)ProjectSettings::get_singleton()->get_setting("sentry/options/debug_printing", (int)SentryOptions::DEBUG_DEFAULT);
	p_options->_init_debug_option(mode);
	p_options->diagnostic_level = (sentry::Level)(int)ProjectSettings::get_singleton()->get_setting("sentry/options/diagnostic_level", p_options->diagnostic_level);

	p_options->sample_rate = ProjectSettings::get_singleton()->get_setting("sentry/options/sample_rate", p_options->sample_rate);
	p_options->traces_sample_rate = ProjectSettings::get_singleton()->get_setting("sentry/options/tracing/traces_sample_rate", p_options->traces_sample_rate);
	p_options->trace_lifecycle = (SentryOptions::TraceLifecycle)(int)ProjectSettings::get_singleton()->get_setting("sentry/options/tracing/trace_lifecycle", p_options->trace_lifecycle);
	const PackedStringArray trace_propagation_targets = ProjectSettings::get_singleton()->get_setting("sentry/options/tracing/trace_propagation_targets", PackedStringArray(p_options->trace_propagation_targets));
	p_options->set_trace_propagation_targets(Array(trace_propagation_targets));
	p_options->propagate_traceparent = ProjectSettings::get_singleton()->get_setting("sentry/options/tracing/propagate_traceparent", p_options->propagate_traceparent);
	p_options->org_id = ProjectSettings::get_singleton()->get_setting("sentry/options/tracing/org_id", p_options->org_id);
	p_options->max_breadcrumbs = ProjectSettings::get_singleton()->get_setting("sentry/options/max_breadcrumbs", p_options->max_breadcrumbs);
	p_options->shutdown_timeout_ms = ProjectSettings::get_singleton()->get_setting("sentry/options/shutdown_timeout_ms", p_options->shutdown_timeout_ms);
	p_options->send_default_pii = ProjectSettings::get_singleton()->get_setting("sentry/options/send_default_pii", p_options->send_default_pii);
	p_options->auto_session_tracking = ProjectSettings::get_singleton()->get_setting("sentry/options/auto_session_tracking", p_options->auto_session_tracking);
	p_options->session_tracking_interval_ms = ProjectSettings::get_singleton()->get_setting("sentry/options/session_tracking_interval_ms", p_options->session_tracking_interval_ms);

	p_options->attach_log = ProjectSettings::get_singleton()->get_setting("sentry/options/attach_log", p_options->attach_log);
	p_options->attach_scene_tree = ProjectSettings::get_singleton()->get_setting("sentry/options/attach_scene_tree", p_options->attach_scene_tree);

	p_options->enable_app_hang_tracking = ProjectSettings::get_singleton()->get_setting("sentry/options/app_hang/tracking", p_options->enable_app_hang_tracking);
	p_options->app_hang_timeout_ms = ProjectSettings::get_singleton()->get_setting("sentry/options/app_hang/timeout_ms", p_options->app_hang_timeout_ms);

	Ref<SentryAutoMetricsOptions> auto_metrics = p_options->get_auto_metrics();
	auto_metrics->set_enable_frame_metrics(ProjectSettings::get_singleton()->get_setting("sentry/auto_metrics/frame_metrics/enabled", auto_metrics->get_enable_frame_metrics()));
	auto_metrics->set_frame_metrics_interval_sec(ProjectSettings::get_singleton()->get_setting("sentry/auto_metrics/frame_metrics/interval_sec", auto_metrics->get_frame_metrics_interval_sec()));
	auto_metrics->set_enable_rendering_metrics(ProjectSettings::get_singleton()->get_setting("sentry/auto_metrics/rendering_metrics/enabled", auto_metrics->get_enable_rendering_metrics()));
	auto_metrics->set_rendering_metrics_interval_sec(ProjectSettings::get_singleton()->get_setting("sentry/auto_metrics/rendering_metrics/interval_sec", auto_metrics->get_rendering_metrics_interval_sec()));
	auto_metrics->set_enable_memory_metrics(ProjectSettings::get_singleton()->get_setting("sentry/auto_metrics/memory_metrics/enabled", auto_metrics->get_enable_memory_metrics()));
	auto_metrics->set_memory_metrics_interval_sec(ProjectSettings::get_singleton()->get_setting("sentry/auto_metrics/memory_metrics/interval_sec", auto_metrics->get_memory_metrics_interval_sec()));
	auto_metrics->set_enable_network_metrics(ProjectSettings::get_singleton()->get_setting("sentry/auto_metrics/network_metrics/enabled", auto_metrics->get_enable_network_metrics()));
	auto_metrics->set_network_metrics_interval_sec(ProjectSettings::get_singleton()->get_setting("sentry/auto_metrics/network_metrics/interval_sec", auto_metrics->get_network_metrics_interval_sec()));

	Ref<SentryGodotLoggerOptions> logger_options = p_options->get_godot_logger();
	logger_options->set_enabled(ProjectSettings::get_singleton()->get_setting("sentry/godot_logger/enabled", logger_options->get_enabled()));
	logger_options->set_include_source_context(ProjectSettings::get_singleton()->get_setting("sentry/godot_logger/include_source_context", logger_options->get_include_source_context()));
	logger_options->set_include_variables(ProjectSettings::get_singleton()->get_setting("sentry/godot_logger/include_variables", logger_options->get_include_variables()));
	logger_options->set_event_mask((int)ProjectSettings::get_singleton()->get_setting("sentry/godot_logger/events", logger_options->get_event_mask()));
	logger_options->set_breadcrumb_mask((int)ProjectSettings::get_singleton()->get_setting("sentry/godot_logger/breadcrumbs", logger_options->get_breadcrumb_mask()));
	logger_options->set_log_mask((int)ProjectSettings::get_singleton()->get_setting("sentry/godot_logger/logs", logger_options->get_log_mask()));

	Ref<SentryLoggerLimits> limits = logger_options->get_limits();
	limits->set_events_per_frame(ProjectSettings::get_singleton()->get_setting("sentry/godot_logger/limits/events_per_frame", limits->get_events_per_frame()));
	limits->set_repeated_error_window_ms(ProjectSettings::get_singleton()->get_setting("sentry/godot_logger/limits/repeated_error_window_ms", limits->get_repeated_error_window_ms()));
	limits->set_throttle_events(ProjectSettings::get_singleton()->get_setting("sentry/godot_logger/limits/throttle_events", limits->get_throttle_events()));
	limits->set_throttle_window_ms(ProjectSettings::get_singleton()->get_setting("sentry/godot_logger/limits/throttle_window_ms", limits->get_throttle_window_ms()));

	Ref<SentryNativeOptions> native = p_options->get_native();
	native->set_crash_reporting_mode(static_cast<SentryNativeOptions::CrashReportingMode>(int(
			ProjectSettings::get_singleton()->get_setting(
					"sentry/native/native_crash_backend/crash_reporting_mode",
					native->get_crash_reporting_mode()))));
	native->set_minidump_mode(static_cast<SentryNativeOptions::MinidumpMode>(int(
			ProjectSettings::get_singleton()->get_setting(
					"sentry/native/native_crash_backend/minidump_mode",
					native->get_minidump_mode()))));
	native->set_thread_stackwalk_mode(static_cast<SentryNativeOptions::ThreadStackwalkMode>(int(
			ProjectSettings::get_singleton()->get_setting(
					"sentry/native/native_crash_backend/thread_stackwalk_mode",
					native->get_thread_stackwalk_mode()))));
	native->set_crash_upload_mode(static_cast<SentryNativeOptions::CrashUploadMode>(int(
			ProjectSettings::get_singleton()->get_setting(
					"sentry/native/native_crash_backend/crash_upload_mode",
					native->get_crash_upload_mode()))));

	Ref<SentryCrashReporterOptions> crash_reporter = p_options->get_crash_reporter();
	crash_reporter->set_enabled(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/enabled", crash_reporter->get_enabled()));
	const String crash_reporter_path_setting = SentryCrashReporterOptions::get_current_path_setting();
	if (!crash_reporter_path_setting.is_empty()) {
		crash_reporter->set_path(ProjectSettings::get_singleton()->get_setting(crash_reporter_path_setting, crash_reporter->get_path()));
	}
	crash_reporter->set_window_title(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/appearance/window_title", crash_reporter->get_window_title()));
	crash_reporter->set_header_text(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/appearance/header_text", crash_reporter->get_header_text()));
	crash_reporter->set_header_description(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/appearance/header_description", crash_reporter->get_header_description()));
	crash_reporter->set_submit_button_label(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/appearance/submit_button_label", crash_reporter->get_submit_button_label()));
	crash_reporter->set_cancel_button_label(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/appearance/cancel_button_label", crash_reporter->get_cancel_button_label()));
	crash_reporter->set_accent_color(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/appearance/accent_color", crash_reporter->get_accent_color()));
	crash_reporter->set_logo_light_theme(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/appearance/logo_light_theme", crash_reporter->get_logo_light_theme()));
	crash_reporter->set_logo_dark_theme(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/appearance/logo_dark_theme", crash_reporter->get_logo_dark_theme()));
	crash_reporter->set_window_closable(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/appearance/window_closable", crash_reporter->get_window_closable()));
	crash_reporter->set_views(ProjectSettings::get_singleton()->get_setting("sentry/crash_reporter/appearance/views", crash_reporter->get_views()));

	p_options->android->set_enable_anr_detection(
			ProjectSettings::get_singleton()->get_setting(
					"sentry/android/application_not_responding/enable_detection",
					p_options->android->get_enable_anr_detection()));
	p_options->android->set_anr_timeout_interval_ms(
			ProjectSettings::get_singleton()->get_setting(
					"sentry/android/application_not_responding/timeout_interval_ms",
					p_options->android->get_anr_timeout_interval_ms()));
	p_options->android->set_attach_anr_thread_dump(
			ProjectSettings::get_singleton()->get_setting(
					"sentry/android/application_not_responding/attach_thread_dump",
					p_options->android->get_attach_anr_thread_dump()));

	p_options->apple->set_enable_metrickit(
			ProjectSettings::get_singleton()->get_setting(
					"sentry/apple/metrickit/enabled",
					p_options->apple->get_enable_metrickit()));

	Ref<SentryWebOptions> web = p_options->get_web();
	web->set_enable_tunnel(ProjectSettings::get_singleton()->get_setting("sentry/web/tunnel/enabled", web->get_enable_tunnel()));
	web->set_tunnel_path(ProjectSettings::get_singleton()->get_setting("sentry/web/tunnel/path", web->get_tunnel_path()));
	web->set_tunnel_base_path(ProjectSettings::get_singleton()->get_setting("sentry/web/tunnel/base_path", web->get_tunnel_base_path()));
	web->set_tunnel_trailing_slash(ProjectSettings::get_singleton()->get_setting("sentry/web/tunnel/trailing_slash", web->get_tunnel_trailing_slash()));

	p_options->attach_screenshot = ProjectSettings::get_singleton()->get_setting("sentry/experimental/attach_screenshot", p_options->attach_screenshot);
	p_options->screenshot_level = (sentry::Level)(int)ProjectSettings::get_singleton()->get_setting("sentry/experimental/screenshot_level", p_options->screenshot_level);
}

void SentryOptions::_init_debug_option(DebugMode p_mode) {
	ERR_FAIL_NULL(OS::get_singleton());
	debug = (p_mode == DebugMode::DEBUG_ON) || (p_mode == DebugMode::DEBUG_AUTO && OS::get_singleton()->is_debug_build());
}

Ref<SentryOptions> SentryOptions::create_from_project_settings() {
	Ref<SentryOptions> options = Ref(memnew(SentryOptions));

	static bool are_settings_defined = false;
	if (!are_settings_defined) {
		_define_project_settings(options);
		are_settings_defined = true;
	}

	_load_project_settings(options);
	return options;
}

void SentryOptions::deprecated_set_logger_messages_as_breadcrumbs(bool p_enabled) {
	WARN_DEPRECATED_MSG("The \"logger_messages_as_breadcrumbs\" option is deprecated. Set the MASK_MESSAGE flag in \"godot_logger.breadcrumb_mask\" instead.");
	BitField<GodotLoggerEventMask> mask = godot_logger->get_breadcrumb_mask();
	if (p_enabled) {
		mask.set_flag(GodotLoggerEventMask::MASK_MESSAGE);
	} else {
		mask.clear_flag(GodotLoggerEventMask::MASK_MESSAGE);
	}
	godot_logger->set_breadcrumb_mask(mask);
}

bool SentryOptions::deprecated_get_app_hang_tracking() const {
	return is_app_hang_tracking_enabled();
}

void SentryOptions::deprecated_set_app_hang_tracking(bool p_enabled) {
	WARN_DEPRECATED_MSG("The \"app_hang_tracking\" option is deprecated. Use \"enable_app_hang_tracking\" instead.");
	set_app_hang_tracking_enabled(p_enabled);
}

double SentryOptions::deprecated_get_app_hang_timeout_sec() const {
	return static_cast<double>(get_app_hang_timeout_ms()) / 1000.0;
}

void SentryOptions::deprecated_set_app_hang_timeout_sec(double p_seconds) {
	WARN_DEPRECATED_MSG("The \"app_hang_timeout_sec\" option is deprecated. Use \"app_hang_timeout_ms\" instead.");
	set_app_hang_timeout_ms(static_cast<int>(Math::round(p_seconds * 1000.0)));
}

void SentryOptions::deprecated_set_logger_enabled(bool p_enabled) {
	WARN_DEPRECATED_MSG("The \"logger_enabled\" option is deprecated. Use \"godot_logger.enabled\" instead.");
	godot_logger->set_enabled(p_enabled);
}

void SentryOptions::deprecated_set_logger_include_source(bool p_enable) {
	WARN_DEPRECATED_MSG("The \"logger_include_source\" option is deprecated. Use \"godot_logger.include_source_context\" instead.");
	godot_logger->set_include_source_context(p_enable);
}

void SentryOptions::deprecated_set_logger_include_variables(bool p_logger_include_variables) {
	WARN_DEPRECATED_MSG("The \"logger_include_variables\" option is deprecated. Use \"godot_logger.include_variables\" instead.");
	godot_logger->set_include_variables(p_logger_include_variables);
}

void SentryOptions::deprecated_set_logger_event_mask(BitField<GodotLoggerEventMask> p_mask) {
	WARN_DEPRECATED_MSG("The \"logger_event_mask\" option is deprecated. Use \"godot_logger.event_mask\" instead.");
	godot_logger->set_event_mask(p_mask);
}

void SentryOptions::deprecated_set_logger_breadcrumb_mask(BitField<GodotLoggerEventMask> p_mask) {
	WARN_DEPRECATED_MSG("The \"logger_breadcrumb_mask\" option is deprecated. Use \"godot_logger.breadcrumb_mask\" instead.");
	godot_logger->set_breadcrumb_mask(p_mask);
}

void SentryOptions::deprecated_set_logger_log_mask(BitField<GodotLoggerEventMask> p_mask) {
	WARN_DEPRECATED_MSG("The \"logger_log_mask\" option is deprecated. Use \"godot_logger.log_mask\" instead.");
	godot_logger->set_log_mask(p_mask);
}

void SentryOptions::deprecated_set_logger_limits(const Ref<SentryLoggerLimits> &p_limits) {
	WARN_DEPRECATED_MSG("The \"logger_limits\" option is deprecated. Use \"godot_logger.limits\" instead.");
	godot_logger->deprecated_set_limits(p_limits);
}

void SentryOptions::deprecated_set_enable_logs(bool) {
	WARN_DEPRECATED_MSG("The \"enable_logs\" option is deprecated, has no effect, and will be removed in version 3.0. Logs are sent when you use SentrySDK.logger or set \"godot_logger.log_mask\".");
}

void SentryOptions::deprecated_set_enable_metrics(bool) {
	WARN_DEPRECATED_MSG("The \"enable_metrics\" option is deprecated, has no effect, and will be removed in version 3.0. Metrics are sent when you use SentrySDK.metrics.");
}

void SentryOptions::set_release(const String &p_release) {
	ERR_FAIL_NULL(ProjectSettings::get_singleton());
	String app_name = ProjectSettings::get_singleton()->get_setting("application/config/name", "Unknown Godot project");
	String app_version = ProjectSettings::get_singleton()->get_setting("application/config/version", "noversion");
	Dictionary format_params;
	format_params["app_name"] = app_name;
	format_params["app_version"] = app_version;
	release = p_release.format(format_params);
}

void SentryOptions::set_environment(const String &p_environment) {
	environment = p_environment.replace("{auto}", environment::detect_godot_environment());
}

void SentryOptions::add_event_processor(const Ref<SentryEventProcessor> &p_processor) {
	ERR_FAIL_COND(p_processor.is_null());
	event_processors.push_back(p_processor);
}

void SentryOptions::add_scope_observer(const Ref<SentryScopeObserver> &p_scope_observer) {
	ERR_FAIL_COND(p_scope_observer.is_null());
	scope_observers.push_back(p_scope_observer);
}

void SentryOptions::remove_event_processor(const Ref<SentryEventProcessor> &p_processor) {
	ERR_FAIL_COND(p_processor.is_null());
	event_processors.erase(p_processor);
}

void SentryOptions::add_default_attachment(const Ref<SentryAttachment> &p_attachment) {
	ERR_FAIL_COND_MSG(p_attachment.is_null(), "Sentry: Internal Error: Can't add null default attachment.");
	ERR_FAIL_COND_MSG(p_attachment->get_path().is_empty(), "Sentry: Internal Error: Default attachments must be file-based (bytes attachments are not allowed).");
	default_attachments.append(p_attachment);
}

void SentryOptions::release_callables() {
	before_send = Callable();
	before_send_transaction = Callable();
	before_send_feedback = Callable();
	before_send_log = Callable();
	before_send_metric = Callable();
	before_capture_screenshot = Callable();
}

void SentryOptions::set_trace_propagation_targets(const Array &p_targets) {
	Array valid_targets;
	for (const Variant &target : p_targets) {
		if (target.get_type() == Variant::STRING) {
			const String text = target;
			if (!text.is_empty()) {
				valid_targets.append(target);
			}
		} else {
			ERR_CONTINUE_MSG(target.get_type() != Variant::OBJECT, "Sentry: trace_propagation_targets entries must be String or RegEx values.");
			Object *object = target;
			RegEx *regex = Object::cast_to<RegEx>(object);
			ERR_CONTINUE_MSG(regex == nullptr, "Sentry: trace_propagation_targets entries must be String or RegEx values.");
			ERR_CONTINUE_MSG(!regex->is_valid(), "Sentry: trace_propagation_targets entries must contain valid regular expressions.");
			valid_targets.append(target);
		}
	}

	trace_propagation_targets = valid_targets;
}

void SentryOptions::_bind_methods() {
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::STRING, "dsn"), set_dsn, get_dsn);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::STRING, "release"), set_release, get_release);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::STRING, "dist"), set_dist, get_dist);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "debug"), set_debug_enabled, is_debug_enabled);
	BIND_PROPERTY(SentryOptions, sentry::make_level_enum_property("diagnostic_level"), set_diagnostic_level, get_diagnostic_level);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::STRING, "environment"), set_environment, get_environment);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::FLOAT, "sample_rate"), set_sample_rate, get_sample_rate);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::FLOAT, "traces_sample_rate"), set_traces_sample_rate, get_traces_sample_rate);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "trace_lifecycle", PROPERTY_HINT_ENUM, "Static,Stream"), set_trace_lifecycle, get_trace_lifecycle);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::ARRAY, "trace_propagation_targets"), set_trace_propagation_targets, get_trace_propagation_targets);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "propagate_traceparent"), set_propagate_traceparent, is_propagate_traceparent_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::STRING, "org_id"), set_org_id, get_org_id);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "max_breadcrumbs"), set_max_breadcrumbs, get_max_breadcrumbs);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "shutdown_timeout_ms", PROPERTY_HINT_RANGE, "0,30000"), set_shutdown_timeout_ms, get_shutdown_timeout_ms);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "auto_session_tracking"), set_auto_session_tracking, is_auto_session_tracking_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "session_tracking_interval_ms", PROPERTY_HINT_RANGE, "0,120000,1,or_greater"), set_session_tracking_interval_ms, get_session_tracking_interval_ms);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "send_default_pii"), set_send_default_pii, is_send_default_pii_enabled);

	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "attach_log"), set_attach_log, is_attach_log_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "attach_screenshot"), set_attach_screenshot, is_attach_screenshot_enabled);
	BIND_PROPERTY(SentryOptions, sentry::make_level_enum_property("screenshot_level"), set_screenshot_level, get_screenshot_level);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "attach_scene_tree"), set_attach_scene_tree, is_attach_scene_tree_enabled);

	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::CALLABLE, "before_send_log"), set_before_send_log, get_before_send_log);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::CALLABLE, "before_send_metric"), set_before_send_metric, get_before_send_metric);

	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "enable_app_hang_tracking"), set_app_hang_tracking_enabled, is_app_hang_tracking_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "app_hang_timeout_ms", PROPERTY_HINT_RANGE, "1000,10000,1"), set_app_hang_timeout_ms, get_app_hang_timeout_ms);

	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::CALLABLE, "before_send"), set_before_send, get_before_send);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::CALLABLE, "before_send_transaction"), set_before_send_transaction, get_before_send_transaction);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::CALLABLE, "before_send_feedback"), set_before_send_feedback, get_before_send_feedback);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::CALLABLE, "before_capture_screenshot"), set_before_capture_screenshot, get_before_capture_screenshot);

	BIND_PROPERTY_READONLY(SentryOptions, PropertyInfo(Variant::OBJECT, "experimental", PROPERTY_HINT_TYPE_STRING, "SentryExperimental", PROPERTY_USAGE_NONE), get_experimental);
	BIND_PROPERTY_READONLY(SentryOptions, PropertyInfo(Variant::OBJECT, "native", PROPERTY_HINT_TYPE_STRING, "SentryNativeOptions", PROPERTY_USAGE_NONE), get_native);
	BIND_PROPERTY_READONLY(SentryOptions, PropertyInfo(Variant::OBJECT, "crash_reporter", PROPERTY_HINT_TYPE_STRING, "SentryCrashReporterOptions", PROPERTY_USAGE_NONE), get_crash_reporter);
	BIND_PROPERTY_READONLY(SentryOptions, PropertyInfo(Variant::OBJECT, "android", PROPERTY_HINT_TYPE_STRING, "SentryAndroidOptions", PROPERTY_USAGE_NONE), get_android);
	BIND_PROPERTY_READONLY(SentryOptions, PropertyInfo(Variant::OBJECT, "apple", PROPERTY_HINT_TYPE_STRING, "SentryAppleOptions", PROPERTY_USAGE_NONE), get_apple);
	BIND_PROPERTY_READONLY(SentryOptions, PropertyInfo(Variant::OBJECT, "web", PROPERTY_HINT_TYPE_STRING, "SentryWebOptions", PROPERTY_USAGE_NONE), get_web);
	BIND_PROPERTY_READONLY(SentryOptions, PropertyInfo(Variant::OBJECT, "auto_metrics", PROPERTY_HINT_TYPE_STRING, "SentryAutoMetricsOptions", PROPERTY_USAGE_NONE), get_auto_metrics);
	BIND_PROPERTY_READONLY(SentryOptions, PropertyInfo(Variant::OBJECT, "godot_logger", PROPERTY_HINT_TYPE_STRING, "SentryGodotLoggerOptions", PROPERTY_USAGE_NONE), get_godot_logger);

	BIND_ENUM_CONSTANT(TRACE_LIFECYCLE_STATIC);
	BIND_ENUM_CONSTANT(TRACE_LIFECYCLE_STREAM);

	{
		using namespace sentry;
		BIND_BITFIELD_FLAG(MASK_NONE);
		BIND_BITFIELD_FLAG(MASK_ERROR);
		BIND_BITFIELD_FLAG(MASK_WARNING);
		BIND_BITFIELD_FLAG(MASK_SCRIPT);
		BIND_BITFIELD_FLAG(MASK_SHADER);
		BIND_BITFIELD_FLAG(MASK_MESSAGE);
	}

	// DEPRECATED: These properties are deprecated and remain for compatibility reasons.
	// TODO: Remove these in January 2027 or in version 3.0.
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "logger_messages_as_breadcrumbs"), deprecated_set_logger_messages_as_breadcrumbs, deprecated_is_logger_messages_as_breadcrumbs_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "app_hang_tracking"), deprecated_set_app_hang_tracking, deprecated_get_app_hang_tracking);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::FLOAT, "app_hang_timeout_sec"), deprecated_set_app_hang_timeout_sec, deprecated_get_app_hang_timeout_sec);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "logger_enabled"), deprecated_set_logger_enabled, deprecated_is_logger_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "logger_include_source"), deprecated_set_logger_include_source, deprecated_is_logger_include_source_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "logger_include_variables"), deprecated_set_logger_include_variables, deprecated_is_logger_include_variables_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "logger_event_mask"), deprecated_set_logger_event_mask, deprecated_get_logger_event_mask);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "logger_breadcrumb_mask"), deprecated_set_logger_breadcrumb_mask, deprecated_get_logger_breadcrumb_mask);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "logger_log_mask"), deprecated_set_logger_log_mask, deprecated_get_logger_log_mask);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::OBJECT, "logger_limits", PROPERTY_HINT_TYPE_STRING, "SentryLoggerLimits", PROPERTY_USAGE_NONE), deprecated_set_logger_limits, deprecated_get_logger_limits);

	// DEPRECATED: Logs and metrics are only sent when their APIs are used, so these toggles are redundant.
	// TODO: Remove these in version 3.0.
	BIND_PROPERTY_SIMPLE_DEPRECATED(SentryOptions, Variant::BOOL, enable_logs);
	BIND_PROPERTY_SIMPLE_DEPRECATED(SentryOptions, Variant::BOOL, enable_metrics);
}

SentryOptions::SentryOptions() {
	native.instantiate();
	crash_reporter.instantiate();
	experimental.instantiate();
	experimental->owner = this;
	android.instantiate();
	apple.instantiate();
	web.instantiate();
	auto_metrics.instantiate();
	godot_logger.instantiate();

	_init_debug_option(DEBUG_DEFAULT);
}

SentryOptions::~SentryOptions() {
	if (experimental.is_valid()) {
		experimental->owner = nullptr;
	}
}

} // namespace sentry
