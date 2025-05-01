#include "android_event.h"

#include "android_string_names.h"

String AndroidEvent::get_id() const {
	return event_id;
}

void AndroidEvent::set_message(const String &p_message) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetMessage), event_id, p_message);
}

String AndroidEvent::get_message() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetMessage), event_id);
}

void AndroidEvent::set_timestamp(const String &p_timestamp) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetTimestamp), event_id, p_timestamp);
}

String AndroidEvent::get_timestamp() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetTimestamp), event_id);
}

String AndroidEvent::get_platform() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetPlatform), event_id);
}

void AndroidEvent::set_level(sentry::Level p_level) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetLevel), event_id, p_level);
}

sentry::Level AndroidEvent::get_level() const {
	ERR_FAIL_NULL_V(android_plugin, sentry::Level::LEVEL_ERROR);
	Variant result = android_plugin->call(ANDROID_SN(eventGetLevel), event_id);
	ERR_FAIL_COND_V(result.get_type() != Variant::INT, sentry::Level::LEVEL_ERROR);
	return sentry::int_to_level(result);
}

void AndroidEvent::set_logger(const String &p_logger) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetLogger), event_id, p_logger);
}

String AndroidEvent::get_logger() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetLogger), event_id);
}

void AndroidEvent::set_release(const String &p_release) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetRelease), event_id, p_release);
}

String AndroidEvent::get_release() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetRelease), event_id);
}

void AndroidEvent::set_dist(const String &p_dist) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetDist), event_id, p_dist);
}

String AndroidEvent::get_dist() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetDist), event_id);
}

void AndroidEvent::set_environment(const String &p_environment) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetEnvironment), event_id, p_environment);
}

String AndroidEvent::get_environment() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetEnvironment), event_id);
}

void AndroidEvent::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetTag), event_id, p_key, p_value);
}

void AndroidEvent::remove_tag(const String &p_key) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventRemoveTag), event_id, p_key);
}

String AndroidEvent::get_tag(const String &p_key) {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetTag), event_id, p_key);
}

void AndroidEvent::add_exception(const String &p_type, const String &p_value, const Vector<StackFrame> &p_frames) {
	ERR_FAIL_NULL(android_plugin);

	String exception_id = android_plugin->call(ANDROID_SN(createException), p_type, p_value);
	ERR_FAIL_COND(exception_id.is_empty());

	for (const StackFrame &frame : p_frames) {
		Dictionary data;
		data["filename"] = frame.filename;
		data["function"] = frame.function;
		data["lineno"] = frame.lineno;
		data["context_line"] = frame.context_line;
		// TODO: Add pre and post lines.
		android_plugin->call(ANDROID_SN(exceptionAppendStackFrame), exception_id, data);
	}

	android_plugin->call(ANDROID_SN(eventAddException), event_id, exception_id);
	android_plugin->call(ANDROID_SN(releaseException), exception_id);
}

AndroidEvent::AndroidEvent(Object *p_android_plugin, String p_event_id) {
	android_plugin = p_android_plugin;
	event_id = p_event_id;
	ERR_FAIL_NULL(android_plugin);
	ERR_FAIL_COND(p_event_id.is_empty());
}

AndroidEvent::~AndroidEvent() {
	if (android_plugin) {
		android_plugin->call(ANDROID_SN(releaseEvent), event_id);
	}
}
