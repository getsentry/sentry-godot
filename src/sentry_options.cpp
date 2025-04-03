#include "sentry_options.h"

#include "sentry/environment.h"
#include "sentry/simple_bind.h"
#include "sentry_sdk.h" // for SentrySDK::Level variant casts

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>

namespace {

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

void _define_setting(const godot::PropertyInfo &p_info, const godot::Variant &p_default, bool p_basic = true) {
	_define_setting(p_info.name, p_default, p_basic);
	Dictionary info = (Dictionary)p_info;
	ProjectSettings::get_singleton()->add_property_info(info);
}

} // unnamed namespace

// *** SentryLoggerLimits

void SentryLoggerLimits::_bind_methods() {
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, events_per_frame);
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, repeated_error_window_ms);
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, throttle_events);
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, throttle_window_ms);
}

// *** SentryOptions

Ref<SentryOptions> SentryOptions::singleton = nullptr;

void SentryOptions::_define_project_settings(const Ref<SentryOptions> &p_options) {
	ERR_FAIL_COND(p_options.is_null());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	_define_setting("sentry/options/enabled", p_options->enabled);
	_define_setting("sentry/options/disabled_in_editor", p_options->disabled_in_editor);
	_define_setting("sentry/options/dsn", p_options->dsn);
	_define_setting("sentry/options/release", p_options->release);
	_define_setting("sentry/options/dist", p_options->dist);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/debug_printing", PROPERTY_HINT_ENUM, "Off,On,Auto"), (int)SentryOptions::DEBUG_DEFAULT);
	_define_setting(sentry::make_level_enum_property("sentry/options/diagnostic_level"), p_options->diagnostic_level);
	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/options/sample_rate", PROPERTY_HINT_RANGE, "0.0,1.0"), p_options->sample_rate);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/max_breadcrumbs", PROPERTY_HINT_RANGE, "0, 500"), p_options->max_breadcrumbs);
	_define_setting("sentry/options/send_default_pii", p_options->send_default_pii);

	_define_setting("sentry/options/attach_log", p_options->attach_log);
	_define_setting("sentry/options/attach_screenshot", p_options->attach_screenshot);
	_define_setting(sentry::make_level_enum_property("sentry/options/screenshot_level"), p_options->screenshot_level);
	_define_setting("sentry/options/attach_scene_tree", p_options->attach_scene_tree, false);

	_define_setting("sentry/options/error_logger/enabled", p_options->error_logger_enabled);
	_define_setting("sentry/options/error_logger/include_source", p_options->error_logger_include_source);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/error_logger/events", PROPERTY_HINT_FLAGS, sentry::GODOT_ERROR_MASK_EXPORT_STRING()), p_options->error_logger_event_mask);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/error_logger/breadcrumbs", PROPERTY_HINT_FLAGS, sentry::GODOT_ERROR_MASK_EXPORT_STRING()), p_options->error_logger_breadcrumb_mask);

	_define_setting("sentry/options/error_logger/limits/parse_lines", p_options->error_logger_limits->parse_lines);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/error_logger/limits/events_per_frame", PROPERTY_HINT_RANGE, "0,20"), p_options->error_logger_limits->events_per_frame);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/error_logger/limits/repeated_error_window_ms", PROPERTY_HINT_RANGE, "0,10000"), p_options->error_logger_limits->repeated_error_window_ms);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/error_logger/limits/throttle_events", PROPERTY_HINT_RANGE, "0,20"), p_options->error_logger_limits->throttle_events);
	_define_setting(PropertyInfo(Variant::INT, "sentry/options/error_logger/limits/throttle_window_ms", PROPERTY_HINT_RANGE, "0,10000"), p_options->error_logger_limits->throttle_window_ms);

	_define_setting(PropertyInfo(Variant::STRING, "sentry/options/configuration_script", PROPERTY_HINT_FILE, "*.gd"), p_options->configuration_script);
}

void SentryOptions::_load_project_settings(const Ref<SentryOptions> &p_options) {
	ERR_FAIL_COND(p_options.is_null());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	String app_name = ProjectSettings::get_singleton()->get_setting("application/config/name", "Unknown Godot project");
	String app_version = ProjectSettings::get_singleton()->get_setting("application/config/version", "noversion");
	String release_str = ProjectSettings::get_singleton()->get_setting("application/config/release", p_options->release);
	Dictionary format_params;
	format_params["app_name"] = app_name;
	format_params["app_version"] = app_version;
	p_options->release = release_str.format(format_params).utf8();

	p_options->enabled = ProjectSettings::get_singleton()->get_setting("sentry/options/enabled", p_options->enabled);
	p_options->disabled_in_editor = ProjectSettings::get_singleton()->get_setting("sentry/options/disabled_in_editor", p_options->disabled_in_editor);
	p_options->dsn = ProjectSettings::get_singleton()->get_setting("sentry/options/dsn", p_options->dsn);
	p_options->dist = ProjectSettings::get_singleton()->get_setting("sentry/options/dist", p_options->dist);

	// DebugMode is only used to represent the debug option in the project settings.
	// The user may also set the `debug` option explicitly in a configuration script.
	DebugMode mode = (DebugMode)(int)ProjectSettings::get_singleton()->get_setting("sentry/options/debug_printing", (int)SentryOptions::DEBUG_DEFAULT);
	p_options->_init_debug_option(mode);
	p_options->diagnostic_level = (sentry::Level)(int)ProjectSettings::get_singleton()->get_setting("sentry/options/diagnostic_level", p_options->diagnostic_level);

	p_options->sample_rate = ProjectSettings::get_singleton()->get_setting("sentry/options/sample_rate", p_options->sample_rate);
	p_options->max_breadcrumbs = ProjectSettings::get_singleton()->get_setting("sentry/options/max_breadcrumbs", p_options->max_breadcrumbs);
	p_options->send_default_pii = ProjectSettings::get_singleton()->get_setting("sentry/options/send_default_pii", p_options->send_default_pii);

	p_options->attach_log = ProjectSettings::get_singleton()->get_setting("sentry/options/attach_log", p_options->attach_log);
	p_options->attach_screenshot = ProjectSettings::get_singleton()->get_setting("sentry/options/attach_screenshot", p_options->attach_screenshot);
	p_options->screenshot_level = (sentry::Level)(int)ProjectSettings::get_singleton()->get_setting("sentry/options/screenshot_level", p_options->screenshot_level);
	p_options->attach_scene_tree = ProjectSettings::get_singleton()->get_setting("sentry/options/attach_scene_tree", p_options->attach_scene_tree);

	p_options->error_logger_enabled = ProjectSettings::get_singleton()->get_setting("sentry/options/error_logger/enabled", p_options->error_logger_enabled);
	p_options->error_logger_include_source = ProjectSettings::get_singleton()->get_setting("sentry/options/error_logger/include_source", p_options->error_logger_include_source);
	p_options->error_logger_event_mask = (int)ProjectSettings::get_singleton()->get_setting("sentry/options/error_logger/events", p_options->error_logger_event_mask);
	p_options->error_logger_breadcrumb_mask = (int)ProjectSettings::get_singleton()->get_setting("sentry/options/error_logger/breadcrumbs", p_options->error_logger_breadcrumb_mask);

	p_options->error_logger_limits->parse_lines = ProjectSettings::get_singleton()->get_setting("sentry/options/error_logger/limits/parse_lines", p_options->error_logger_limits->parse_lines);
	p_options->error_logger_limits->events_per_frame = ProjectSettings::get_singleton()->get_setting("sentry/options/error_logger/limits/events_per_frame", p_options->error_logger_limits->events_per_frame);
	p_options->error_logger_limits->repeated_error_window_ms = ProjectSettings::get_singleton()->get_setting("sentry/options/error_logger/limits/repeated_error_window_ms", p_options->error_logger_limits->repeated_error_window_ms);
	p_options->error_logger_limits->throttle_events = ProjectSettings::get_singleton()->get_setting("sentry/options/error_logger/limits/throttle_events", p_options->error_logger_limits->throttle_events);
	p_options->error_logger_limits->throttle_window_ms = ProjectSettings::get_singleton()->get_setting("sentry/options/error_logger/limits/throttle_window_ms", p_options->error_logger_limits->throttle_window_ms);

	p_options->configuration_script = ProjectSettings::get_singleton()->get_setting("sentry/options/configuration_script", p_options->configuration_script);
}

void SentryOptions::_init_debug_option(DebugMode p_mode) {
	ERR_FAIL_NULL(OS::get_singleton());
	debug = (p_mode == DebugMode::DEBUG_ON) || (p_mode == DebugMode::DEBUG_AUTO && OS::get_singleton()->is_debug_build());
}

void SentryOptions::create_singleton() {
	singleton = Ref(memnew(SentryOptions));

	static bool are_settings_defined = false;
	if (!are_settings_defined) {
		_define_project_settings(singleton);
		are_settings_defined = true;
	}

	_load_project_settings(singleton);
}

void SentryOptions::destroy_singleton() {
	singleton = Ref<SentryOptions>();
}

void SentryOptions::set_error_logger_limits(const Ref<SentryLoggerLimits> &p_limits) {
	error_logger_limits = p_limits;
	// Ensure limits are initialized.
	if (error_logger_limits.is_null()) {
		error_logger_limits.instantiate();
	}
}

void SentryOptions::_bind_methods() {
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "enabled"), set_enabled, is_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "disabled_in_editor"), set_disabled_in_editor, is_disabled_in_editor);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::STRING, "dsn"), set_dsn, get_dsn);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::STRING, "release"), set_release, get_release);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::STRING, "dist"), set_dist, get_dist);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "debug"), set_debug_enabled, is_debug_enabled);
	BIND_PROPERTY(SentryOptions, sentry::make_level_enum_property("diagnostic_level"), set_diagnostic_level, get_diagnostic_level);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::STRING, "environment"), set_environment, get_environment);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::FLOAT, "sample_rate"), set_sample_rate, get_sample_rate);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "max_breadcrumbs"), set_max_breadcrumbs, get_max_breadcrumbs);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "send_default_pii"), set_send_default_pii, is_send_default_pii_enabled);

	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "attach_log"), set_attach_log, is_attach_log_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "attach_screenshot"), set_attach_screenshot, is_attach_screenshot_enabled);
	BIND_PROPERTY(SentryOptions, sentry::make_level_enum_property("screenshot_level"), set_screenshot_level, get_screenshot_level);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "attach_scene_tree"), set_attach_scene_tree, is_attach_scene_tree_enabled);

	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "error_logger_enabled"), set_error_logger_enabled, is_error_logger_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "error_logger_include_source"), set_error_logger_include_source, is_error_logger_include_source_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "error_logger_event_mask"), set_error_logger_event_mask, get_error_logger_event_mask);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "error_logger_breadcrumb_mask"), set_error_logger_breadcrumb_mask, get_error_logger_breadcrumb_mask);

	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::OBJECT, "error_logger_limits", PROPERTY_HINT_TYPE_STRING, "SentryLoggerLimits", PROPERTY_USAGE_NONE), set_error_logger_limits, get_error_logger_limits);

	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::CALLABLE, "before_send"), set_before_send, get_before_send);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::CALLABLE, "on_crash"), set_on_crash, get_on_crash);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::CALLABLE, "before_capture_screenshot"), set_before_capture_screenshot, get_before_capture_screenshot);

	{
		using namespace sentry;
		BIND_BITFIELD_FLAG(MASK_NONE);
		BIND_BITFIELD_FLAG(MASK_ERROR);
		BIND_BITFIELD_FLAG(MASK_WARNING);
		BIND_BITFIELD_FLAG(MASK_SCRIPT);
		BIND_BITFIELD_FLAG(MASK_SHADER);
	}
}

SentryOptions::SentryOptions() {
	error_logger_limits.instantiate(); // Ensure limits are initialized.
	environment = sentry::environment::detect_godot_environment();
	_init_debug_option(DEBUG_DEFAULT);
}

SentryOptions::~SentryOptions() {
}
