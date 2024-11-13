#include "sentry_options.h"

#include <godot_cpp/classes/project_settings.hpp>

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
	dsn = String(ProjectSettings::get_singleton()->get_setting("sentry/config/dsn", String(dsn))).utf8();
	debug = ProjectSettings::get_singleton()->get_setting("sentry/config/debug", debug);
	sample_rate = ProjectSettings::get_singleton()->get_setting("sentry/config/sample_rate", sample_rate);
	attach_log = ProjectSettings::get_singleton()->get_setting("sentry/config/attach_log", attach_log);
	max_breadcrumbs = ProjectSettings::get_singleton()->get_setting("sentry/config/max_breadcrumbs", max_breadcrumbs);

	error_logger_enabled = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/enabled", error_logger_enabled);
	error_logger_max_lines = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/max_lines", error_logger_max_lines);
	error_logger_include_source = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/include_source", error_logger_include_source);
	error_logger_breadcrumb_mask = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/breadcrumbs", error_logger_breadcrumb_mask);
	error_logger_event_mask = ProjectSettings::get_singleton()->get_setting("sentry/config/error_logger/events", error_logger_event_mask);
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
	_define_setting("sentry/config/dsn", String(dsn));
	_define_setting("sentry/config/release", String(release));
	_define_setting("sentry/config/debug", debug);
	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/config/sample_rate", PROPERTY_HINT_RANGE, "0.0,1.0"), sample_rate);
	_define_setting("sentry/config/attach_log", attach_log);
	_define_setting(PropertyInfo(Variant::INT, "sentry/config/max_breadcrumbs", PROPERTY_HINT_RANGE, "0, 500"), max_breadcrumbs);

	_define_setting("sentry/config/error_logger/enabled", error_logger_enabled);
	_define_setting("sentry/config/error_logger/max_lines", error_logger_max_lines);
	_define_setting("sentry/config/error_logger/include_source", error_logger_include_source);
	_define_setting(PropertyInfo(Variant::INT, "sentry/config/error_logger/breadcrumbs", PROPERTY_HINT_FLAGS, sentry::GODOT_ERROR_MASK_EXPORT_STRING), error_logger_breadcrumb_mask);
	_define_setting(PropertyInfo(Variant::INT, "sentry/config/error_logger/events", PROPERTY_HINT_FLAGS, sentry::GODOT_ERROR_MASK_EXPORT_STRING), error_logger_event_mask);
}

SentryOptions::SentryOptions() {
	singleton = this;

	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	_define_project_settings();
	_load_project_settings();
}

SentryOptions::~SentryOptions() {
	singleton = nullptr;
}
