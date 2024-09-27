#include "sentry_singleton.h"

#include "sentry.h"
#include "settings.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

Sentry *Sentry::singleton = nullptr;

void Sentry::capture_message_event(const String &p_text, Level p_level, const String &p_logger) {
	// TODO: Specify log level.
	sentry_value_t event = sentry_value_new_message_event(
			(sentry_level_t)p_level,
			p_logger.utf8().get_data(),
			p_text.utf8().get_data());
	sentry_capture_event(event);
}

void Sentry::_bind_methods() {
	BIND_ENUM_CONSTANT(LEVEL_DEBUG);
	BIND_ENUM_CONSTANT(LEVEL_INFO);
	BIND_ENUM_CONSTANT(LEVEL_WARNING);
	BIND_ENUM_CONSTANT(LEVEL_ERROR);
	BIND_ENUM_CONSTANT(LEVEL_FATAL);

	ClassDB::bind_method(D_METHOD("capture_message_event", "text", "level", "logger"), &Sentry::capture_message_event, DEFVAL(LEVEL_INFO), DEFVAL(""));
}

Sentry::Sentry() {
	sentry_options_t *options = sentry_options_new();

	sentry_options_set_dsn(options, Settings::get_singleton()->get_dsn());
	sentry_options_set_backend(options, NULL);
	sentry_options_set_database_path(options, Settings::get_singleton()->get_db_path());
	sentry_options_set_release(options, Settings::get_singleton()->get_release());

	sentry_options_set_debug(options, 1);
	sentry_options_set_environment(options, get_environment());

	sentry_init(options);

	singleton = this;
}

Sentry::~Sentry() {
	singleton = nullptr;
	sentry_close();
}
