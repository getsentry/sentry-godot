#include "native_log.h"

#include "native_util.h"

#include "sentry/logging/print.h"

namespace sentry::native {

LogLevel NativeLog::get_level() const {
	sentry_value_t level_value = sentry_value_get_by_key(native_log, "level");
	const char *level_cstr = sentry_value_as_string(level_value);
	if (level_cstr == NULL) {
		WARN_PRINT("Sentry: Invalid native structured log level value.");
		return LOG_LEVEL_INFO;
	}

	if (strcmp(level_cstr, "trace") == 0) {
		return LOG_LEVEL_TRACE;
	} else if (strcmp(level_cstr, "debug") == 0) {
		return LOG_LEVEL_DEBUG;
	} else if (strcmp(level_cstr, "info") == 0) {
		return LOG_LEVEL_INFO;
	} else if (strcmp(level_cstr, "warn") == 0) {
		return LOG_LEVEL_WARN;
	} else if (strcmp(level_cstr, "error") == 0) {
		return LOG_LEVEL_ERROR;
	} else if (strcmp(level_cstr, "fatal") == 0) {
		return LOG_LEVEL_FATAL;
	} else {
		WARN_PRINT("Sentry: Unexpected native structured log level value \"" + String(level_cstr) + "\"");
		return LOG_LEVEL_INFO;
	}
}

void NativeLog::set_level(LogLevel p_level) {
	switch (p_level) {
		case LOG_LEVEL_TRACE: {
			sentry_value_set_by_key(native_log, "level", sentry_value_new_string("trace"));
		} break;
		case LOG_LEVEL_DEBUG: {
			sentry_value_set_by_key(native_log, "level", sentry_value_new_string("debug"));
		} break;
		case LOG_LEVEL_INFO: {
			sentry_value_set_by_key(native_log, "level", sentry_value_new_string("info"));
		} break;
		case LOG_LEVEL_WARN: {
			sentry_value_set_by_key(native_log, "level", sentry_value_new_string("warn"));
		} break;
		case LOG_LEVEL_ERROR: {
			sentry_value_set_by_key(native_log, "level", sentry_value_new_string("error"));
		} break;
		case LOG_LEVEL_FATAL: {
			sentry_value_set_by_key(native_log, "level", sentry_value_new_string("fatal"));
		} break;
		default: {
			WARN_PRINT("Sentry: Unexpected structured log level enum value.");
		} break;
	}
}

String NativeLog::get_body() const {
	sentry_value_t body = sentry_value_get_by_key(native_log, "body");
	return String::utf8(sentry_value_as_string(body));
}

void NativeLog::set_body(const String &p_body) {
	sentry_value_set_by_key(native_log, "body", sentry_value_new_string(p_body.utf8()));
}

Variant NativeLog::get_attribute(const String &p_name) const {
	return sentry_value_get_attribute(native_log, p_name);
}

void NativeLog::set_attribute(const String &p_name, const Variant &p_value) {
	sentry_value_set_attribute(native_log, p_name, p_value);
}

void NativeLog::add_attributes(const Dictionary &p_attributes) {
	sentry_value_add_attributes(native_log, p_attributes);
}

void NativeLog::remove_attribute(const String &p_name) {
	sentry_value_t attributes = sentry_value_get_by_key(native_log, "attributes");
	sentry_value_remove_by_key(attributes, p_name.utf8());
}

NativeLog::NativeLog() {
	native_log = sentry_value_new_object();
	// NOTE: Required for ClassDB registration and compilation to succeed.
	ERR_PRINT("This constructor is not intended for runtime use.");
}

NativeLog::NativeLog(sentry_value_t p_native_log) :
		native_log(p_native_log) {
	sentry_value_incref(native_log);
}

NativeLog::~NativeLog() {
	sentry_value_decref(native_log);
}

} // namespace sentry::native
