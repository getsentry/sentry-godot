#include "android_event.h"

#include "android_string_names.h"

String AndroidEvent::get_id() const {
	return event_id;
}

void AndroidEvent::set_message(const String &p_message) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetMessage), p_message);
}

String AndroidEvent::get_message() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetMessage));
}

void AndroidEvent::set_timestamp(const String &p_timestamp) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetTimestamp), p_timestamp);
}

String AndroidEvent::get_timestamp() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetTimestamp));
}

String AndroidEvent::get_platform() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetPlatform));
}

void AndroidEvent::set_level(sentry::Level p_level) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetLevel), p_level);
}

sentry::Level AndroidEvent::get_level() const {
	ERR_FAIL_NULL_V(android_plugin, sentry::Level::LEVEL_ERROR);
	Variant result = android_plugin->call(ANDROID_SN(eventGetLevel));
	ERR_FAIL_COND_V(result.get_type() != Variant::INT, sentry::Level::LEVEL_ERROR);
	return sentry::int_to_level(result);
}

void AndroidEvent::set_logger(const String &p_logger) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetLogger), p_logger);
}

String AndroidEvent::get_logger() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetLogger));
}

void AndroidEvent::set_release(const String &p_release) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetRelease), p_release);
}

String AndroidEvent::get_release() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetRelease));
}

void AndroidEvent::set_dist(const String &p_dist) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetDist), p_dist);
}

String AndroidEvent::get_dist() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetDist));
}

void AndroidEvent::set_environment(const String &p_environment) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetEnvironment), p_environment);
}

String AndroidEvent::get_environment() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetEnvironment));
}

void AndroidEvent::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetTag), p_key, p_value);
}

void AndroidEvent::remove_tag(const String &p_key) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventRemoveTag), p_key);
}

godot::String AndroidEvent::get_tag(const String &p_key) {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetTag), p_key);
}

AndroidEvent::AndroidEvent(Object *p_android_plugin, String p_event_id) {
	ERR_FAIL_NULL(android_plugin);
	ERR_FAIL_COND(p_event_id.is_empty());
	android_plugin = p_android_plugin;
	event_id = p_event_id;
}

AndroidEvent::~AndroidEvent() {
	if (android_plugin) {
		android_plugin->call(ANDROID_SN(releaseEvent), event_id);
	}
}
