#include "settings.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

Settings *Settings::singleton = nullptr;

void Settings::_load_config() {
	// Read project config since ProjectSettings singleton may not be initialized at this point.
	Ref<ConfigFile> conf = memnew(ConfigFile);
	Error err = conf->load("res://project.godot");
	ERR_FAIL_COND_MSG(err > 0, "Sentry: Fatal error: Failed to open project config.");

	String app_name = conf->get_value("application", "config/name", "Unknown Godot project");
	String app_version = conf->get_value("application", "config/version", "noversion");
	release = (app_name + "@" + app_version).utf8();

	dsn = ((String)conf->get_value("sentry", "config/dsn", "")).utf8();
	debug_printing = (bool)conf->get_value("sentry", "config/debug_printing", false);
	sample_rate = (double)conf->get_value("sentry", "config/sample_rate", 1.0);
}

void Settings::_define_setting(const String &p_setting, const Variant &p_default) {
	if (!ProjectSettings::get_singleton()->has_setting(p_setting)) {
		ProjectSettings::get_singleton()->set(p_setting, p_default);
	}
	ProjectSettings::get_singleton()->set_initial_value(p_setting, p_default);
}

void Settings::_define_setting(const godot::PropertyInfo &p_info, const godot::Variant &p_default) {
	_define_setting(p_info.name, p_default);
	Dictionary info = (Dictionary)p_info;
	ProjectSettings::get_singleton()->add_property_info(info);
}

void Settings::_define_project_settings() {
	_define_setting("sentry/config/dsn", String(""));
	_define_setting("sentry/config/debug_printing", false);
	_define_setting(PropertyInfo(Variant::FLOAT, "sentry/config/sample_rate", PROPERTY_HINT_RANGE, "0.0,1.0"), 1.0);
}

Settings::Settings() {
	singleton = this;

	if (ProjectSettings::get_singleton()) {
		_define_project_settings();
	}

	_load_config();
}

Settings::~Settings() {
	singleton = nullptr;
}
