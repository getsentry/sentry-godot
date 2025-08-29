#include "android_event.h"

#include "android_string_names.h"

namespace sentry {

String AndroidEvent::get_id() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetId), event_handle);
}

void AndroidEvent::set_message(const String &p_message) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetMessage), event_handle, p_message);
}

String AndroidEvent::get_message() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetMessage), event_handle);
}

void AndroidEvent::set_timestamp(const Ref<SentryTimestamp> &p_timestamp) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetTimestamp), event_handle, p_timestamp.is_valid() ? p_timestamp->get_microseconds_since_unix_epoch() : 0);
}

Ref<SentryTimestamp> AndroidEvent::get_timestamp() const {
	ERR_FAIL_NULL_V(android_plugin, nullptr);
	int64_t micros = android_plugin->call(ANDROID_SN(eventGetTimestamp), event_handle);
	return SentryTimestamp::from_microseconds_since_unix_epoch(micros);
}

String AndroidEvent::get_platform() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetPlatform), event_handle);
}

void AndroidEvent::set_level(sentry::Level p_level) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetLevel), event_handle, p_level);
}

sentry::Level AndroidEvent::get_level() const {
	ERR_FAIL_NULL_V(android_plugin, sentry::Level::LEVEL_ERROR);
	Variant result = android_plugin->call(ANDROID_SN(eventGetLevel), event_handle);
	ERR_FAIL_COND_V(result.get_type() != Variant::INT, sentry::Level::LEVEL_ERROR);
	return sentry::int_to_level(result);
}

void AndroidEvent::set_logger(const String &p_logger) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetLogger), event_handle, p_logger);
}

String AndroidEvent::get_logger() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetLogger), event_handle);
}

void AndroidEvent::set_release(const String &p_release) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetRelease), event_handle, p_release);
}

String AndroidEvent::get_release() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetRelease), event_handle);
}

void AndroidEvent::set_dist(const String &p_dist) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetDist), event_handle, p_dist);
}

String AndroidEvent::get_dist() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetDist), event_handle);
}

void AndroidEvent::set_environment(const String &p_environment) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetEnvironment), event_handle, p_environment);
}

String AndroidEvent::get_environment() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetEnvironment), event_handle);
}

void AndroidEvent::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventSetTag), event_handle, p_key, p_value);
}

void AndroidEvent::remove_tag(const String &p_key) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(eventRemoveTag), event_handle, p_key);
}

String AndroidEvent::get_tag(const String &p_key) {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventGetTag), event_handle, p_key);
}

void AndroidEvent::merge_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't merge context with an empty key.");
	android_plugin->call(ANDROID_SN(eventMergeContext), event_handle, p_key, p_value);
}

void AndroidEvent::add_exception(const Exception &p_exception) {
	ERR_FAIL_NULL(android_plugin);

	int32_t exception_handle = android_plugin->call(ANDROID_SN(createException), p_exception.type, p_exception.value);

	for (const StackFrame &frame : p_exception.frames) {
		Dictionary data;
		data["filename"] = frame.filename;
		data["function"] = frame.function;
		data["lineno"] = frame.lineno;
		data["in_app"] = frame.in_app;
		data["platform"] = frame.platform;
		if (!frame.context_line.is_empty()) {
			data["context_line"] = frame.context_line;
			data["pre_context"] = frame.pre_context;
			data["post_context"] = frame.post_context;
		}
		android_plugin->call(ANDROID_SN(exceptionAppendStackFrame), exception_handle, data);
	}

	android_plugin->call(ANDROID_SN(eventAddException), event_handle, exception_handle);
	android_plugin->call(ANDROID_SN(releaseException), exception_handle);
}

bool AndroidEvent::is_crash() const {
	ERR_FAIL_NULL_V(android_plugin, false);
	return android_plugin->call(ANDROID_SN(eventIsCrash), event_handle);
}

String AndroidEvent::to_json() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(eventToJson), event_handle);
}

AndroidEvent::AndroidEvent(Object *p_android_plugin, int32_t p_event_handle) {
	android_plugin = p_android_plugin;
	event_handle = p_event_handle;
	ERR_FAIL_NULL(android_plugin);
}

AndroidEvent::~AndroidEvent() {
	if (android_plugin && !is_borrowed) {
		android_plugin->call(ANDROID_SN(releaseEvent), event_handle);
	}
}

} // namespace sentry
