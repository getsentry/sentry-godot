#include "javascript_sdk.h"

#include "sentry/sentry_options.h"

#include <godot_cpp/classes/java_script_bridge.hpp>

namespace sentry::javascript {

void JavaScriptSDK::set_context(const String &p_key, const Dictionary &p_value) {
	WARN_PRINT("JavaScriptSDK::set_context() not implemented");
	// TODO: Implement JavaScript SDK context setting
}

void JavaScriptSDK::remove_context(const String &p_key) {
	WARN_PRINT("JavaScriptSDK::remove_context() not implemented");
	// TODO: Implement JavaScript SDK context removal
}

void JavaScriptSDK::set_tag(const String &p_key, const String &p_value) {
	WARN_PRINT("JavaScriptSDK::set_tag() not implemented");
	// TODO: Implement JavaScript SDK tag setting
}

void JavaScriptSDK::remove_tag(const String &p_key) {
	WARN_PRINT("JavaScriptSDK::remove_tag() not implemented");
	// TODO: Implement JavaScript SDK tag removal
}

void JavaScriptSDK::set_user(const Ref<SentryUser> &p_user) {
	WARN_PRINT("JavaScriptSDK::set_user() not implemented");
	// TODO: Implement JavaScript SDK user setting
}

void JavaScriptSDK::remove_user() {
	WARN_PRINT("JavaScriptSDK::remove_user() not implemented");
	// TODO: Implement JavaScript SDK user removal
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
	WARN_PRINT("JavaScriptSDK::log() not implemented");
	// TODO: Implement JavaScript SDK logging
}

String JavaScriptSDK::capture_message(const String &p_message, Level p_level) {
	sentry_bridge->call("captureMessage", String(level_as_cstring(p_level)));
	// TODO: Implement JavaScript SDK message capture
	return String();
}

String JavaScriptSDK::get_last_event_id() {
	WARN_PRINT("JavaScriptSDK::get_last_event_id() not implemented");
	// TODO: Implement JavaScript SDK last event ID retrieval
	return String();
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
	sentry_bridge->call("init",
			SentryOptions::get_singleton()->get_dsn(),
			SentryOptions::get_singleton()->get_release());
	// TODO: Implement JavaScript SDK initialization
}

void JavaScriptSDK::close() {
	WARN_PRINT("JavaScriptSDK::close() not implemented");
	// TODO: Implement JavaScript SDK shutdown
}

bool JavaScriptSDK::is_enabled() const {
	WARN_PRINT("JavaScriptSDK::is_enabled() not implemented");
	// TODO: Implement JavaScript SDK enabled check
	return false;
}

JavaScriptSDK::JavaScriptSDK() {
	ERR_FAIL_NULL(JavaScriptBridge::get_singleton());

	sentry_bridge = JavaScriptBridge::get_singleton()->get_interface("SentryBridge");
	ERR_FAIL_COND_MSG(sentry_bridge.is_null(), "Failed to initialize JavaScript SDK - SentryBridge interface not found.");
}

JavaScriptSDK::~JavaScriptSDK() {
}

} //namespace sentry::javascript
