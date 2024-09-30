#include "sentry_singleton.h"

#include "sentry.h"
#include "sentry_util.h"
#include "settings.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

Sentry *Sentry::singleton = nullptr;

godot::CharString Sentry::get_level_cstring(Level p_level) {
	switch (p_level) {
		case LEVEL_DEBUG: {
			return "debug";
		} break;
		case LEVEL_INFO: {
			return "info";
		} break;
		case LEVEL_WARNING: {
			return "warning";
		} break;
		case LEVEL_ERROR: {
			return "error";
		} break;
		case LEVEL_FATAL: {
			return "fatal";
		} break;
	}
	return "invalid";
}

void Sentry::capture_message(const String &p_message, Level p_level, const String &p_logger) {
	sentry_value_t event = sentry_value_new_message_event(
			(sentry_level_t)p_level,
			p_logger.utf8().get_data(),
			p_message.utf8().get_data());
	sentry_capture_event(event);
}

void Sentry::add_breadcrumb(const godot::String &p_message, const godot::String &p_category, Level p_level,
		const godot::String &p_type, const godot::Dictionary &p_data) {
	sentry_value_t crumb = sentry_value_new_breadcrumb(p_type.utf8().ptr(), p_message.utf8().ptr());
	sentry_value_set_by_key(crumb, "category", sentry_value_new_string(p_category.utf8().ptr()));
	sentry_value_set_by_key(crumb, "level", sentry_value_new_string(get_level_cstring(p_level)));
	sentry_value_set_by_key(crumb, "data", SentryUtil::variant_to_sentry_value(p_data));
	sentry_add_breadcrumb(crumb);
}

void Sentry::_bind_methods() {
	BIND_ENUM_CONSTANT(LEVEL_DEBUG);
	BIND_ENUM_CONSTANT(LEVEL_INFO);
	BIND_ENUM_CONSTANT(LEVEL_WARNING);
	BIND_ENUM_CONSTANT(LEVEL_ERROR);
	BIND_ENUM_CONSTANT(LEVEL_FATAL);

	ClassDB::bind_method(D_METHOD("capture_message", "message", "level", "logger"), &Sentry::capture_message, DEFVAL(LEVEL_INFO), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("add_breadcrumb", "message", "category", "level", "type", "data"), &Sentry::add_breadcrumb, DEFVAL(LEVEL_INFO), DEFVAL("default"), DEFVAL(Dictionary()));
}

Sentry::Sentry() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());
	ERR_FAIL_NULL(Settings::get_singleton());

	sentry_options_t *options = sentry_options_new();
	sentry_options_set_dsn(options, Settings::get_singleton()->get_dsn());

	// Establish handler path.
	String handler_fn;
#ifdef LINUX_ENABLED
	handler_fn = "crashpad_handler";
#elif WINDOWS_ENABLED
	handler_fn = "crashpad_handler.exe";
#endif
	String handler_path = OS::get_singleton()->get_executable_path() + "/" + handler_fn;
	if (!FileAccess::file_exists(handler_path)) {
		handler_path = ProjectSettings::get_singleton()->globalize_path("res://addons/sentrysdk/bin/" + handler_fn);
	}
	if (FileAccess::file_exists(handler_path)) {
		sentry_options_set_handler_path(options, handler_path.utf8());
	} else {
		ERR_PRINT("Sentry: Failed to locate crash handler (crashpad)");
		sentry_options_set_backend(options, NULL);
	}

	sentry_options_set_database_path(options, (OS::get_singleton()->get_user_data_dir() + "/sentry").utf8());
	sentry_options_set_sample_rate(options, Settings::get_singleton()->get_sample_rate());
	sentry_options_set_release(options, Settings::get_singleton()->get_release());
	sentry_options_set_debug(options, Settings::get_singleton()->is_debug_printing_enabled());
	sentry_options_set_environment(options, get_environment());

	sentry_init(options);

	singleton = this;
}

Sentry::~Sentry() {
	singleton = nullptr;
	sentry_close();
}
