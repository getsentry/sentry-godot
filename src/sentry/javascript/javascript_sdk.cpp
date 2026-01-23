#include "javascript_sdk.h"

#include "sentry/javascript/javascript_breadcrumb.h"
#include "sentry/javascript/javascript_event.h"
#include "sentry/javascript/javascript_util.h"
#include "sentry/sentry_options.h"

#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/classes/json.hpp>

namespace sentry::javascript {

void JavaScriptSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("setContext", p_key, JSON::stringify(p_value), String(), false);
}

void JavaScriptSDK::remove_context(const String &p_key) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("removeContext", p_key);
}

void JavaScriptSDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("setTag", p_key, p_value);
}

void JavaScriptSDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("removeTag", p_key);
}

void JavaScriptSDK::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	if (p_user.is_null()) {
		js_sentry_bridge()->call("removeUser");
	} else {
		js_sentry_bridge()->call("setUser",
				p_user->get_id(),
				p_user->get_username(),
				p_user->get_email(),
				p_user->get_ip_address());
	}
}

void JavaScriptSDK::remove_user() {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("removeUser");
}

Ref<SentryBreadcrumb> JavaScriptSDK::create_breadcrumb() {
	return memnew(JavaScriptBreadcrumb);
}

void JavaScriptSDK::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	if (p_breadcrumb.is_null()) {
		return;
	}
	JavaScriptBreadcrumb *crumb = Object::cast_to<JavaScriptBreadcrumb>(p_breadcrumb.ptr());
	ERR_FAIL_NULL(crumb);
	js_sentry_bridge()->call("addBreadcrumb", crumb->get_js_object());
}

void JavaScriptSDK::log(LogLevel p_level, const String &p_body, const Dictionary &p_attributes) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());

	String attr_value = attributes_to_json(p_attributes);

	switch (p_level) {
		case LOG_LEVEL_TRACE: {
			js_sentry_bridge()->call("logTrace", p_body, attr_value);
		} break;
		case LOG_LEVEL_DEBUG: {
			js_sentry_bridge()->call("logDebug", p_body, attr_value);
		} break;
		case LOG_LEVEL_INFO: {
			js_sentry_bridge()->call("logInfo", p_body, attr_value);
		} break;
		case LOG_LEVEL_WARN: {
			js_sentry_bridge()->call("logWarn", p_body, attr_value);
		} break;
		case LOG_LEVEL_ERROR: {
			js_sentry_bridge()->call("logError", p_body, attr_value);
		} break;
		case LOG_LEVEL_FATAL: {
			js_sentry_bridge()->call("logFatal", p_body, attr_value);
		} break;
	}
}

String JavaScriptSDK::capture_message(const String &p_message, Level p_level) {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), String());
	return js_sentry_bridge()->call("captureMessage", p_message, level_as_string(p_level));
}

String JavaScriptSDK::get_last_event_id() {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), String());
	return js_sentry_bridge()->call("lastEventId");
}

Ref<SentryEvent> JavaScriptSDK::create_event() {
	return memnew(JavaScriptEvent);
}

String JavaScriptSDK::capture_event(const Ref<SentryEvent> &p_event) {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), String());
	JavaScriptEvent *ev = Object::cast_to<JavaScriptEvent>(p_event.ptr());
	ERR_FAIL_NULL_V(ev, String());
	return js_sentry_bridge()->call("captureEvent", ev->get_js_object());
}

void JavaScriptSDK::capture_feedback(const Ref<SentryFeedback> &p_feedback) {
	WARN_PRINT("JavaScriptSDK::capture_feedback() not implemented");
	// TODO: Implement JavaScript SDK feedback capture
}

void JavaScriptSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	WARN_PRINT("JavaScriptSDK::add_attachment() not implemented");
	// TODO: Implement JavaScript SDK attachment addition
}

void JavaScriptSDK::init(const PackedStringArray &p_global_attachments, const Callable &p_configuration_callback) {
	ERR_FAIL_COND(js_sentry_bridge().is_null());

	js_sentry_bridge()->call("init",
			SentryOptions::get_singleton()->get_dsn(),
			SentryOptions::get_singleton()->is_debug_enabled(),
			SentryOptions::get_singleton()->get_release(),
			SentryOptions::get_singleton()->get_dist(),
			SentryOptions::get_singleton()->get_environment(),
			SentryOptions::get_singleton()->get_sample_rate(),
			SentryOptions::get_singleton()->get_max_breadcrumbs(),
			SentryOptions::get_singleton()->get_enable_logs());
}

void JavaScriptSDK::close() {
	ERR_FAIL_COND(js_sentry_bridge().is_null());
	js_sentry_bridge()->call("close");
}

bool JavaScriptSDK::is_enabled() const {
	ERR_FAIL_COND_V(js_sentry_bridge().is_null(), false);
	return js_sentry_bridge()->call("isEnabled");
}

JavaScriptSDK::JavaScriptSDK() {
}

JavaScriptSDK::~JavaScriptSDK() {
}

} //namespace sentry::javascript
