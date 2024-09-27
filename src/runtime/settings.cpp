#include "settings.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

Settings *Settings::singleton = nullptr;

void Settings::_load_config() {
	// Read project config since ProjectSettings singleton is not initialized at this point.
	Ref<ConfigFile> conf = memnew(ConfigFile);
	Error err = conf->load("res://project.godot");
	ERR_FAIL_COND_MSG(err > 0, "Sentry: Fatal error: Failed to open project config.");

	String app_name = conf->get_value("application", "config/name", "Unknown Godot project");
	String app_version = conf->get_value("application", "config/version", "noversion");
	release = (app_name + "@" + app_version).utf8();

	dsn = ((String)conf->get_value("sentry", "config/dsn", "")).utf8();

	// TODO: Use user folder instead.
	// See https://docs.sentry.io/platforms/native/configuration/options/#database-path
	db_path = ".sentry-native";
}

Settings::Settings() {
	singleton = this;

	_load_config();
}

Settings::~Settings() {
	singleton = nullptr;
}
