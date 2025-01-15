#include "native_event.h"

#include "sentry/level.h"
#include "sentry/native/native_util.h"

#include <sentry.h>

namespace {

inline void _sentry_value_set_or_remove_string_by_key(sentry_value_t value, const char *k, const String &v) {
	if (v.is_empty()) {
		sentry_value_remove_by_key(value, k);
	} else {
		sentry_value_set_by_key(value, k, sentry_value_new_string(v.utf8()));
	}
}

} // unnamed namespace

String NativeEvent::get_id() const {
	sentry_value_t id = sentry_value_get_by_key(native_event, "event_id");
	return sentry_value_as_string(id);
}

void NativeEvent::set_message(const String &p_message) {
	if (p_message.is_empty()) {
		sentry_value_remove_by_key(native_event, "message");
	} else {
		sentry_value_t message = sentry_value_get_by_key(native_event, "message");
		if (sentry_value_is_null(message)) {
			message = sentry_value_new_object();
			sentry_value_set_by_key(native_event, "message", message);
		}
		sentry_value_set_by_key(message, "formatted", sentry_value_new_string(p_message.utf8()));
	}
}

String NativeEvent::get_message() const {
	sentry_value_t message = sentry_value_get_by_key(native_event, "message");
	sentry_value_t formatted = sentry_value_get_by_key(message, "formatted");
	return sentry_value_as_string(formatted);
}

void NativeEvent::set_timestamp(const String &p_timestamp) {
	_sentry_value_set_or_remove_string_by_key(native_event, "timestamp", p_timestamp);
}

String NativeEvent::get_timestamp() const {
	sentry_value_t timestamp = sentry_value_get_by_key(native_event, "timestamp");
	return sentry_value_as_string(timestamp);
}

String NativeEvent::get_platform() const {
	sentry_value_t platform = sentry_value_get_by_key(native_event, "platform");
	return sentry_value_as_string(platform);
}

void NativeEvent::set_level(sentry::Level p_level) {
	sentry_value_set_by_key(native_event, "level",
			sentry_value_new_string(sentry::native::level_to_cstring(p_level)));
}

sentry::Level NativeEvent::get_level() const {
	sentry_value_t value = sentry_value_get_by_key(native_event, "level");
	return sentry::native::cstring_to_level(sentry_value_as_string(value));
}

void NativeEvent::set_logger(const String &p_logger) {
	_sentry_value_set_or_remove_string_by_key(native_event, "logger", p_logger);
}

String NativeEvent::get_logger() const {
	sentry_value_t logger = sentry_value_get_by_key(native_event, "logger");
	return sentry_value_as_string(logger);
}

void NativeEvent::set_release(const String &p_release) {
	_sentry_value_set_or_remove_string_by_key(native_event, "release", p_release);
}

String NativeEvent::get_release() const {
	sentry_value_t release = sentry_value_get_by_key(native_event, "release");
	return sentry_value_as_string(release);
}

void NativeEvent::set_dist(const String &p_dist) {
	_sentry_value_set_or_remove_string_by_key(native_event, "dist", p_dist);
}

String NativeEvent::get_dist() const {
	sentry_value_t dist = sentry_value_get_by_key(native_event, "dist");
	return sentry_value_as_string(dist);
}

void NativeEvent::set_environment(const String &p_environment) {
	_sentry_value_set_or_remove_string_by_key(native_event, "environment", p_environment);
}

String NativeEvent::get_environment() const {
	sentry_value_t environment = sentry_value_get_by_key(native_event, "environment");
	return sentry_value_as_string(environment);
}

NativeEvent::NativeEvent(sentry_value_t p_native_event) {
	native_event = p_native_event;
}

NativeEvent::NativeEvent() :
		NativeEvent(sentry_value_new_event()) {
}

NativeEvent::~NativeEvent() {
	sentry_value_decref(native_event);
}