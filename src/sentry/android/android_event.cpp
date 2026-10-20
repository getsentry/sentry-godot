#include "android_event.h"

#include "android_string_names.h"
#include "android_util.h"
#include "sentry/logging/print.h"

#include <godot_cpp/classes/os.hpp>

namespace sentry::android {

String AndroidEvent::get_id() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(eventGetId), event_handle);
}

void AndroidEvent::set_message(const String &p_message) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetMessage), event_handle, p_message);
}

String AndroidEvent::get_message() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(eventGetMessage), event_handle);
}

void AndroidEvent::set_timestamp(const Ref<SentryTimestamp> &p_timestamp) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetTimestamp), event_handle, p_timestamp.is_valid() ? p_timestamp->get_microseconds_since_unix_epoch() : 0);
}

Ref<SentryTimestamp> AndroidEvent::get_timestamp() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, nullptr);
	int64_t micros = plugin->call(ANDROID_SN(eventGetTimestamp), event_handle);
	return SentryTimestamp::from_microseconds_since_unix_epoch(micros);
}

String AndroidEvent::get_platform() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(eventGetPlatform), event_handle);
}

void AndroidEvent::set_level(sentry::Level p_level) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetLevel), event_handle, p_level);
}

sentry::Level AndroidEvent::get_level() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, sentry::Level::LEVEL_ERROR);
	Variant result = plugin->call(ANDROID_SN(eventGetLevel), event_handle);
	ERR_FAIL_COND_V(result.get_type() != Variant::INT, sentry::Level::LEVEL_ERROR);
	return sentry::int_to_level(result);
}

void AndroidEvent::set_logger(const String &p_logger) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetLogger), event_handle, p_logger);
}

String AndroidEvent::get_logger() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(eventGetLogger), event_handle);
}

void AndroidEvent::set_release(const String &p_release) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetRelease), event_handle, p_release);
}

String AndroidEvent::get_release() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(eventGetRelease), event_handle);
}

void AndroidEvent::set_dist(const String &p_dist) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetDist), event_handle, p_dist);
}

String AndroidEvent::get_dist() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(eventGetDist), event_handle);
}

void AndroidEvent::set_environment(const String &p_environment) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetEnvironment), event_handle, p_environment);
}

String AndroidEvent::get_environment() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(eventGetEnvironment), event_handle);
}

void AndroidEvent::set_tag(const String &p_key, const String &p_value) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetTag), event_handle, p_key, p_value);
}

void AndroidEvent::remove_tag(const String &p_key) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventRemoveTag), event_handle, p_key);
}

String AndroidEvent::get_tag(const String &p_key) {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(eventGetTag), event_handle, p_key);
}

void AndroidEvent::set_user(const Ref<SentryUser> &p_user) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	if (p_user.is_valid()) {
		plugin->call(ANDROID_SN(eventSetUser),
				event_handle,
				p_user->get_id(),
				p_user->get_username(),
				p_user->get_email(),
				p_user->get_ip_address());
	} else {
		plugin->call(ANDROID_SN(eventRemoveUser), event_handle);
	}
}

void AndroidEvent::set_fingerprint(const PackedStringArray &p_fingerprint) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetFingerprint), event_handle, p_fingerprint);
}

void AndroidEvent::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set context with an empty key.");
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetContext),
			event_handle, p_key, sanitize_variant(p_value));
}

void AndroidEvent::merge_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't merge context with an empty key.");
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventMergeContext),
			event_handle, p_key, sanitize_variant(p_value));
}

void AndroidEvent::add_exception(const Exception &p_exception) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);

	Array stacktrace_frames;
	for (const StackFrame &frame : p_exception.frames) {
		Dictionary frame_data;
		frame_data["filename"] = frame.filename;
		frame_data["function"] = frame.function;
		frame_data["lineno"] = frame.lineno;
		frame_data["in_app"] = frame.in_app;
		frame_data["platform"] = frame.platform;
		if (!frame.context_line.is_empty()) {
			frame_data["context_line"] = frame.context_line;
			frame_data["pre_context"] = frame.pre_context;
			frame_data["post_context"] = frame.post_context;
		}

		if (!frame.vars.is_empty()) {
			Dictionary variables;
			for (auto var : frame.vars) {
				variables[var.first] = sanitize_variant(var.second);
			}
			frame_data["vars"] = variables;
		}

		stacktrace_frames.append(frame_data);
	}

	uint64_t thread_id = godot::OS::get_singleton()->get_thread_caller_id();
	bool is_main = godot::OS::get_singleton()->get_main_thread_id() == thread_id;

	Dictionary thread_data;
	thread_data["thread_id"] = thread_id;
	thread_data["main"] = is_main;
	thread_data["crashed"] = true;
	thread_data["current"] = true;
	thread_data["frames"] = stacktrace_frames;

	plugin->call(ANDROID_SN(eventAddException), event_handle, p_exception.type, p_exception.value, thread_id);
	plugin->call(ANDROID_SN(eventAddThreadStackTrace), event_handle, thread_data);
}

int AndroidEvent::get_exception_count() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, 0);
	return plugin->call(ANDROID_SN(eventGetExceptionCount), event_handle);
}

void AndroidEvent::set_exception_value(int p_index, const String &p_value) {
	ENSURE_ANDROID_PLUGIN(plugin, _android_plugin);
	plugin->call(ANDROID_SN(eventSetExceptionValue), event_handle, p_index, p_value);
}

String AndroidEvent::get_exception_value(int p_index) const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(eventGetExceptionValue), event_handle, p_index);
}

bool AndroidEvent::is_crash() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, false);
	return plugin->call(ANDROID_SN(eventIsCrash), event_handle);
}

String AndroidEvent::to_json() const {
	ENSURE_ANDROID_PLUGIN_V(plugin, _android_plugin, String());
	return plugin->call(ANDROID_SN(eventToJson), event_handle);
}

AndroidEvent::AndroidEvent(util::ObjectHandle<Object> p_android_plugin, int32_t p_event_handle) :
		_android_plugin(p_android_plugin), event_handle(p_event_handle) {
}

AndroidEvent::~AndroidEvent() {
	if (Object *plugin = _android_plugin.get(); plugin && !is_borrowed) {
		plugin->call(ANDROID_SN(releaseEvent), event_handle);
	}
}

} //namespace sentry::android
