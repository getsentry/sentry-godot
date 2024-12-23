#include "native_event.h"

#include "sentry/level.h"
#include "sentry/native/native_util.h"

#include <sentry.h>

String NativeEvent::get_id() const {
	sentry_value_t id = sentry_value_get_by_key(native_event, "id");
	return sentry_value_as_string(id);
}

void NativeEvent::set_timestamp(const String &p_timestamp) {
	sentry_value_set_by_key(native_event, "timestamp",
			sentry_value_new_string(p_timestamp.utf8()));
}

String NativeEvent::get_timestamp() const {
	sentry_value_t timestamp = sentry_value_get_by_key(native_event, "timestamp");
	return sentry_value_as_string(timestamp);
}

void NativeEvent::set_level(sentry::Level p_level) {
	sentry_value_set_by_key(native_event, "level",
			sentry_value_new_string(sentry::native::level_to_cstring(p_level)));
}

sentry::Level NativeEvent::get_level() const {
	sentry_value_t value = sentry_value_get_by_key(native_event, "level");
	sentry_level_t level = (sentry_level_t)sentry_value_as_int32(value);
	return sentry::native::native_to_level(level);
}

NativeEvent::NativeEvent(sentry_value_t p_native_event) {
	native_event = p_native_event;
	sentry_value_incref(p_native_event);
}

NativeEvent::NativeEvent() :
		NativeEvent(sentry_value_new_event()) {
	sentry_value_decref(native_event);
}

NativeEvent::~NativeEvent() {
	sentry_value_decref(native_event);
}
