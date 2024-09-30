#include "sentry_settings.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

SentrySettings *SentrySettings::singleton = nullptr;

void SentrySettings::_load_config() {
	// Read project config since ProjectSettings singleton may not be initialized at this point.
	Ref<ConfigFile> conf = memnew(ConfigFile);
	Error err = conf->load("res://project.godot");
	ERR_FAIL_COND_MSG(err > 0, "Sentry: Fatal error: Failed to open project config.");

	// Prepare release name.
	String app_name = conf->get_value("application", "config/name", "Unknown Godot project");
	String app_version = conf->get_value("application", "config/version", "noversion");
	String release_str = ((String)conf->get_value("sentry", "config/release", String(release)));
	Dictionary format_params;
	format_params["app_name"] = app_name;
	format_params["app_version"] = app_version;
	release = release_str.format(format_params).utf8();

	sentry_enabled = (bool)conf->get_value("sentry", "config/sentry_enabled", sentry_enabled);
	dsn = ((String)conf->get_value("sentry", "config/dsn", String(dsn))).utf8();
	debug_printing = (bool)conf->get_value("sentry", "config/debug_printing", debug_printing);
	sample_rate = (double)conf->get_value("sentry", "config/sample_rate", sample_rate);
	attach_log = (bool)conf->get_value("sentry", "config/attach_log", attach_log);
}

void SentrySettings::_define_setting(const String &p_setting, const Variant &p_default) {
	if (!ProjectSettings::get_singleton()->has_setting(p_setting)) {
		ProjectSettings::get_singleton()->set(p_setting, p_default);
	}
	ProjectSettings::get_singleton()->set_initial_value(p_setting, p_default);
}

void SentrySettings::_define_setting(const godot::PropertyInfo &p_info, const godot::Variant &p_default) {
	_define_setting(p_info.name, p_default);
	Dictionary info = (Dictionary)p_info;
	ProjectSettings::get_singleton()->add_property_info(info);
}

void SentrySettings::_define_project_settings() {
	_define_setting("sentry/config/sentry_enabled", sentry_enabled);
	_define_setting("sentry/config/dsn", String(dsn));
	_define_setting("sentry/config/release", String(release));
	_define_setting("sentry/config/debug_printing", debug_printing);
	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/config/sample_rate", PROPERTY_HINT_RANGE, "0.0,1.0"), sample_rate);
	_define_setting("sentry/config/attach_log", attach_log);
}

SentrySettings::SentrySettings() {
	singleton = this;

	if (ProjectSettings::get_singleton()) {
		_define_project_settings();
	}

	_load_config();
}

SentrySettings::~SentrySettings() {
	singleton = nullptr;
}
