#include "settings_migrations.h"

#include "sentry/godot_error_types.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace {

void _define_internal_project_setting(const String &p_name, const Variant &p_default_value) {
	if (!ProjectSettings::get_singleton()->has_setting(p_name)) {
		ProjectSettings::get_singleton()->set_setting(p_name, p_default_value);
	}
	ProjectSettings::get_singleton()->set_initial_value(p_name, p_default_value);
	ProjectSettings::get_singleton()->set_as_internal(p_name, true);
}

bool _has_any_persisted_sentry_settings() {
	const TypedArray<Dictionary> properties = ProjectSettings::get_singleton()->get_property_list();
	for (int i = 0; i < properties.size(); ++i) {
		const String name = ((Dictionary)properties[i]).get("name", String());
		if (name.begins_with("sentry/")) {
			return true;
		}
	}
	return false;
}

void _set_project_setting_and_save(const String &p_key, const Variant &p_value) {
	ProjectSettings::get_singleton()->set_setting(p_key, p_value);
	if (Engine::get_singleton()->is_editor_hint()) {
		ProjectSettings::get_singleton()->call_deferred("save");
	}
}

void _rename_setting(const String &p_old_name, const String &p_new_name) {
	if (ProjectSettings::get_singleton()->has_setting(p_old_name)) {
		Variant value = ProjectSettings::get_singleton()->get_setting(p_old_name);
		ProjectSettings::get_singleton()->set_setting(p_new_name, value);
		ProjectSettings::get_singleton()->set_setting(p_old_name, Variant());
	}
}

void _migrate_messages_as_breadcrumbs() {
	const String old_setting = "sentry/logger/messages_as_breadcrumbs";
	const String mask_setting = "sentry/logger/breadcrumbs";

	const bool was_enabled = ProjectSettings::get_singleton()->get_setting(old_setting, true);
	int mask = ProjectSettings::get_singleton()->get_setting(
			mask_setting, int(sentry::GodotLoggerEventMask::MASK_ALL));

	if (was_enabled) {
		mask |= sentry::GodotLoggerEventMask::MASK_MESSAGE;
	} else {
		mask &= ~sentry::GodotLoggerEventMask::MASK_MESSAGE;
	}
	ProjectSettings::get_singleton()->set_setting(mask_setting, mask);
	ProjectSettings::get_singleton()->set_setting(old_setting, Variant());
}

void _migrate_to_v1() {
	_rename_setting("sentry/experimental/enable_logs", "sentry/options/enable_logs");
	_migrate_messages_as_breadcrumbs();
}

} // unnamed namespace

namespace sentry {

constexpr static int SCHEMA_VERSION = 1;

void run_project_settings_migrations() {
	const String schema_version_key = "sentry/schema_version";

	const bool has_version = ProjectSettings::get_singleton()->has_setting(schema_version_key);
	const bool is_fresh_install = !has_version && !_has_any_persisted_sentry_settings();

	_define_internal_project_setting(schema_version_key, 0);

	if (is_fresh_install) {
		_set_project_setting_and_save(schema_version_key, SCHEMA_VERSION);
		return;
	}

	const int from_version = ProjectSettings::get_singleton()->get_setting(schema_version_key, 0);

	if (from_version > SCHEMA_VERSION) {
		UtilityFunctions::push_warning("Sentry: Project settings are from a newer SDK version than the current version. Migration is not supported.");
		return;
	}

	if (from_version < 1) {
		_migrate_to_v1();
	}

	if (from_version < SCHEMA_VERSION) {
		_set_project_setting_and_save(schema_version_key, SCHEMA_VERSION);
		UtilityFunctions::print(
				vformat("Sentry: Project settings migrated from schema version %d to %d.",
						from_version, SCHEMA_VERSION));
	}
}

} //namespace sentry
