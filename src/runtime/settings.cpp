#include "settings.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/error_macros.hpp>

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
}

void Settings::_define_setting(const String &p_setting, const Variant &p_default) {
	if (!ProjectSettings::get_singleton()->has_setting(p_setting)) {
		ProjectSettings::get_singleton()->set(p_setting, p_default);
	}
	ProjectSettings::get_singleton()->set_initial_value(p_setting, p_default);
}

void Settings::_define_project_settings() {
	_define_setting("sentry/config/dsn", String(""));
	_define_setting("sentry/config/debug_printing", false);
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
