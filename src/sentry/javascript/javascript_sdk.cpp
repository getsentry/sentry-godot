#include "javascript_sdk.h"

#include "sentry/sentry_options.h"

#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/classes/json.hpp>

namespace sentry::javascript {

Ref<JavaScriptObject> JavaScriptSDK::_get_bridge() const {
	if (unlikely(_bridge.is_null())) {
		ERR_FAIL_NULL_V(JavaScriptBridge::get_singleton(), Ref<JavaScriptObject>());
		const_cast<JavaScriptSDK *>(this)->_bridge = JavaScriptBridge::get_singleton()->get_interface("SentryBridge");
		ERR_FAIL_COND_V_MSG(_bridge.is_null(), Ref<JavaScriptObject>(), "SentryBridge JS interface not found!");
	}
	return _bridge;
}

void JavaScriptSDK::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(_get_bridge().is_null());
	_get_bridge()->call("setContext", p_key, JSON::stringify(p_value), String(), false);
}

void JavaScriptSDK::remove_context(const String &p_key) {
	ERR_FAIL_COND(_get_bridge().is_null());
	_get_bridge()->call("removeContext", p_key);
}

void JavaScriptSDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(_get_bridge().is_null());
	_get_bridge()->call("setTag", p_key, p_value);
}

void JavaScriptSDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND(_get_bridge().is_null());
	_get_bridge()->call("removeTag", p_key);
}

void JavaScriptSDK::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_COND(_get_bridge().is_null());
	if (p_user.is_null()) {
		_get_bridge()->call("removeUser");
	} else {
		_get_bridge()->call("setUser",
				p_user->get_id(),
				p_user->get_username(),
				p_user->get_email(),
				p_user->get_ip_address());
	}
}

void JavaScriptSDK::remove_user() {
	_get_bridge()->call("removeUser");
}

Ref<SentryBreadcrumb> JavaScriptSDK::create_breadcrumb() {
	WARN_PRINT("JavaScriptSDK::create_breadcrumb() not implemented");
	// TODO: Implement JavaScript SDK breadcrumb creation
	return Ref<SentryBreadcrumb>();
}

void JavaScriptSDK::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	WARN_PRINT("JavaScriptSDK::add_breadcrumb() not implemented");
	// TODO: Implement JavaScript SDK breadcrumb addition
}

void JavaScriptSDK::log(LogLevel p_level, const String &p_body, const Dictionary &p_attributes) {
	ERR_FAIL_COND(_get_bridge().is_null());

	String attr_value;
	if (!p_attributes.is_empty()) {
		attr_value = JSON::stringify(p_attributes);
	}

	switch (p_level) {
		case LOG_LEVEL_TRACE: {
			_get_bridge()->call("logTrace", p_body, attr_value);
		} break;
		case LOG_LEVEL_DEBUG: {
			_get_bridge()->call("logDebug", p_body, attr_value);
		} break;
		case LOG_LEVEL_INFO: {
			_get_bridge()->call("logInfo", p_body, attr_value);
		} break;
		case LOG_LEVEL_WARN: {
			_get_bridge()->call("logWarn", p_body, attr_value);
		} break;
		case LOG_LEVEL_ERROR: {
			_get_bridge()->call("logError", p_body, attr_value);
		} break;
		case LOG_LEVEL_FATAL: {
			_get_bridge()->call("logFatal", p_body, attr_value);
		} break;
	}
}

String JavaScriptSDK::capture_message(const String &p_message, Level p_level) {
	ERR_FAIL_COND_V(_get_bridge().is_null(), String());
	return _get_bridge()->call("captureMessage", p_message, level_as_string(p_level));
}

String JavaScriptSDK::get_last_event_id() {
	ERR_FAIL_COND_V(_get_bridge().is_null(), String());
	return _get_bridge()->call("lastEventId");
}

Ref<SentryEvent> JavaScriptSDK::create_event() {
	WARN_PRINT("JavaScriptSDK::create_event() not implemented");
	// TODO: Implement JavaScript SDK event creation
	return Ref<SentryEvent>();
}

String JavaScriptSDK::capture_event(const Ref<SentryEvent> &p_event) {
	WARN_PRINT("JavaScriptSDK::capture_event() not implemented");
	// TODO: Implement JavaScript SDK event capture
	return String();
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
	ERR_FAIL_COND(_get_bridge().is_null());

	_get_bridge()->call("init",
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
	WARN_PRINT("JavaScriptSDK::close() not implemented");
	// TODO: Implement JavaScript SDK shutdown
}

bool JavaScriptSDK::is_enabled() const {
	ERR_FAIL_COND_V(_get_bridge().is_null(), false);
	return false;
	// return _get_bridge()->call("isEnabled");
}

JavaScriptSDK::JavaScriptSDK() {
}

JavaScriptSDK::~JavaScriptSDK() {
}

} //namespace sentry::javascript
