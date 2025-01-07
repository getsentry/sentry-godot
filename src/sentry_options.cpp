#include "sentry_options.h"
#include "sentry/simple_bind.h"

#include <godot_cpp/classes/project_settings.hpp>

// *** SentryLoggerLimits

void SentryLoggerLimits::_bind_methods() {
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, events_per_frame);
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, repeated_error_window_ms);
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, throttle_events);
	BIND_PROPERTY_SIMPLE(SentryLoggerLimits, Variant::INT, throttle_window_ms);
}

// *** SentryOptions

SentryOptions *SentryOptions::singleton = nullptr;

void SentryOptions::_load_project_settings() {
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	// Prepare release name.
	String app_name = ProjectSettings::get_singleton()->get_setting("application/config/name", "Unknown Godot project");
	String app_version = ProjectSettings::get_singleton()->get_setting("application/config/version", "noversion");
	String release_str = ProjectSettings::get_singleton()->get_setting("application/config/release", String(release));
	Dictionary format_params;
	format_params["app_name"] = app_name;
	format_params["app_version"] = app_version;
	release = release_str.format(format_params).utf8();

	enabled = ProjectSettings::get_singleton()->get_setting("sentry/config/enabled", enabled);
	disabled_in_editor = ProjectSettings::get_singleton()->get_setting("sentry/config/disabled_in_editor", disabled_in_editor);
	dsn = String(ProjectSettings::get_singleton()->get_setting("sentry/config/dsn", String(dsn))).utf8();
	debug = ProjectSettings::get_singleton()->get_setting("sentry/config/debug", debug);
	sample_rate = ProjectSettings::get_singleton()->get_setting("sentry/config/sample_rate", sample_rate);
	attach_log = ProjectSettings::get_singleton()->get_setting("sentry/config/attach_log", attach_log);
	max_breadcrumbs = ProjectSettings::get_singleton()->get_setting("sentry/config/max_breadcrumbs", max_breadcrumbs);
	send_default_pii = ProjectSettings::get_singleton()->get_setting("sentry/config/send_default_pii", send_default_pii);

	error_logger_enabled = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/enabled", error_logger_enabled);
	error_logger_include_source = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/include_source", error_logger_include_source);
	error_logger_event_mask = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/events", error_logger_event_mask);
	error_logger_breadcrumb_mask = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/breadcrumbs", error_logger_breadcrumb_mask);

	error_logger_limits->parse_lines = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/limits/parse_lines", error_logger_limits->parse_lines);
	error_logger_limits->events_per_frame = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/limits/events_per_frame", error_logger_limits->events_per_frame);
	error_logger_limits->repeated_error_window_ms = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/limits/repeated_error_window_ms", error_logger_limits->repeated_error_window_ms);
	error_logger_limits->throttle_events = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/limits/throttle_events", error_logger_limits->throttle_events);
	error_logger_limits->throttle_window_ms = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/limits/throttle_window_ms", error_logger_limits->throttle_window_ms);
}

void SentryOptions::_define_setting(const String &p_setting, const Variant &p_default, bool p_basic) {
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	if (!ProjectSettings::get_singleton()->has_setting(p_setting)) {
		ProjectSettings::get_singleton()->set(p_setting, p_default);
	}
	ProjectSettings::get_singleton()->set_initial_value(p_setting, p_default);
	ProjectSettings::get_singleton()->set_as_basic(p_setting, p_basic);
	ProjectSettings::get_singleton()->set_restart_if_changed(p_setting, false);

	// Preserve order of the defined settings.
	if (config_value_order == 0) {
		config_value_order = ProjectSettings::get_singleton()->get_order(p_setting);
	}
	ProjectSettings::get_singleton()->set_order(p_setting, config_value_order);
	config_value_order++;
}

void SentryOptions::_define_setting(const godot::PropertyInfo &p_info, const godot::Variant &p_default, bool p_basic) {
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	_define_setting(p_info.name, p_default, p_basic);
	Dictionary info = (Dictionary)p_info;
	ProjectSettings::get_singleton()->add_property_info(info);
}

void SentryOptions::_define_project_settings() {
	_define_setting("sentry/config/enabled", enabled);
	_define_setting("sentry/config/disabled_in_editor", disabled_in_editor);
	_define_setting("sentry/config/dsn", String(dsn));
	_define_setting("sentry/config/release", String(release));
	_define_setting("sentry/config/debug", debug);
	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/config/sample_rate", PROPERTY_HINT_RANGE, "0.0,1.0"), sample_rate);
	_define_setting("sentry/config/attach_log", attach_log);
	_define_setting(PropertyInfo(Variant::INT, "sentry/config/max_breadcrumbs", PROPERTY_HINT_RANGE, "0, 500"), max_breadcrumbs);
	_define_setting("sentry/config/send_default_pii", send_default_pii);

	_define_setting("sentry/config/error_logger/enabled", error_logger_enabled);
	_define_setting("sentry/config/error_logger/include_source", error_logger_include_source);
	_define_setting(PropertyInfo(Variant::INT, "sentry/config/error_logger/events", PROPERTY_HINT_FLAGS, sentry::GODOT_ERROR_MASK_EXPORT_STRING()), error_logger_event_mask);
	_define_setting(PropertyInfo(Variant::INT, "sentry/config/error_logger/breadcrumbs", PROPERTY_HINT_FLAGS, sentry::GODOT_ERROR_MASK_EXPORT_STRING()), error_logger_breadcrumb_mask);

	_define_setting("sentry/config/error_logger/limits/parse_lines", error_logger_limits->parse_lines);
	_define_setting(PropertyInfo(Variant::INT, "sentry/config/error_logger/limits/events_per_frame", PROPERTY_HINT_RANGE, "0,20"), error_logger_limits->events_per_frame);
	_define_setting(PropertyInfo(Variant::INT, "sentry/config/error_logger/limits/repeated_error_window_ms", PROPERTY_HINT_RANGE, "0,10000"), error_logger_limits->repeated_error_window_ms);
	_define_setting(PropertyInfo(Variant::INT, "sentry/config/error_logger/limits/throttle_events", PROPERTY_HINT_RANGE, "0,20"), error_logger_limits->throttle_events);
	_define_setting(PropertyInfo(Variant::INT, "sentry/config/error_logger/limits/throttle_window_ms", PROPERTY_HINT_RANGE, "0,10000"), error_logger_limits->throttle_window_ms);
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
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "debug"), set_debug_enabled, is_debug_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::FLOAT, "sample_rate"), set_sample_rate, get_sample_rate);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "attach_log"), set_attach_log, is_attach_log_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "max_breadcrumbs"), set_max_breadcrumbs, get_max_breadcrumbs);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "send_default_pii"), set_send_default_pii, is_send_default_pii_enabled);

	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "error_logger_enabled"), set_error_logger_enabled, is_error_logger_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::BOOL, "error_logger_include_source"), set_error_logger_include_source, is_error_logger_include_source_enabled);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "error_logger_event_mask"), set_error_logger_event_mask, get_error_logger_event_mask);
	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::INT, "error_logger_breadcrumb_mask"), set_error_logger_breadcrumb_mask, get_error_logger_breadcrumb_mask);

	BIND_PROPERTY(SentryOptions, PropertyInfo(Variant::OBJECT, "error_logger_limits", PROPERTY_HINT_TYPE_STRING, "SentryLoggerLimits", PROPERTY_USAGE_NONE), set_error_logger_limits, get_error_logger_limits);
}

SentryOptions::SentryOptions() {
	singleton = this;
	error_logger_limits.instantiate(); // Ensure limits are initialized.

	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	_define_project_settings();
	_load_project_settings();
}

SentryOptions::~SentryOptions() {
	singleton = nullptr;
}
