#include "android_sdk.h"

#include "android_event.h"
#include "android_string_names.h"
#include "sentry/util/print.h"

#include <godot_cpp/classes/engine.hpp>

namespace sentry {

void AndroidSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(setContext), p_key, p_value);
}

void AndroidSDK::remove_context(const String &p_key) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(removeContext), p_key);
}

void AndroidSDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(setTag), p_key, p_value);
}

void AndroidSDK::remove_tag(const String &p_key) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(removeTag), p_key);
}

void AndroidSDK::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(setUser),
			p_user->get_id(),
			p_user->get_username(),
			p_user->get_email(),
			p_user->get_ip_address());
}

void AndroidSDK::remove_user() {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(removeUser));
}

void AndroidSDK::add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
		const String &p_type, const Dictionary &p_data) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(addBreadcrumb), p_message, p_category, p_level, p_type, p_data);
}

String AndroidSDK::capture_message(const String &p_message, Level p_level) {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(captureMessage), p_message, p_level);
}

String AndroidSDK::get_last_event_id() {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(getLastEventId));
}

String AndroidSDK::capture_error(const String &p_type, const String &p_value, Level p_level, const Vector<StackFrame> &p_frames) {
	ERR_FAIL_NULL_V(android_plugin, String());
	// TODO: implement!
	WARN_PRINT_ONCE("capture_error not implemented for Android SDK.");
	return String();
}

Ref<SentryEvent> AndroidSDK::create_event() {
	ERR_FAIL_NULL_V(android_plugin, nullptr);
	String event_id = android_plugin->call(ANDROID_SN(createEvent));
	ERR_FAIL_COND_V(event_id.is_empty(), nullptr);
	Ref<AndroidEvent> event = memnew(AndroidEvent(android_plugin, event_id));
	return event;
}

String AndroidSDK::capture_event(const Ref<SentryEvent> &p_event) {
	ERR_FAIL_NULL_V(android_plugin, String());
	ERR_FAIL_COND_V(p_event.is_null(), String());
	String event_id = p_event->get_id();
	ERR_FAIL_COND_V(event_id.is_empty(), String());
	android_plugin->call(ANDROID_SN(captureEvent), event_id);
	return event_id;
}

void AndroidSDK::initialize() {
	ERR_FAIL_NULL(android_plugin);

	sentry::util::print_debug("Initializing Sentry Android SDK");
	android_plugin->call("initialize",
			SentryOptions::get_singleton()->get_dsn(),
			SentryOptions::get_singleton()->is_debug_enabled(),
			SentryOptions::get_singleton()->get_release(),
			SentryOptions::get_singleton()->get_dist(),
			SentryOptions::get_singleton()->get_environment(),
			SentryOptions::get_singleton()->get_sample_rate(),
			SentryOptions::get_singleton()->get_max_breadcrumbs());
}

AndroidSDK::AndroidSDK() {
	android_plugin = Engine::get_singleton()->get_singleton("SentryAndroidGodotPlugin");
	AndroidStringNames::create_singleton();
}

AndroidSDK::~AndroidSDK() {
	AndroidStringNames::destroy_singleton();
}

} //namespace sentry
