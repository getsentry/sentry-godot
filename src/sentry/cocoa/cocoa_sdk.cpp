#include "cocoa_sdk.h"

#include "sentry/common_defs.h"
#include "sentry/processing/process_event.h"
#include "sentry/sentry_attachment.h"
#include "sentry/util/print.h"
#include "sentry_objc_wrapper.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

namespace sentry {

void CocoaSDK::set_context(const String &p_key, const Dictionary &p_value) {
	objc_wrapper::set_context_objc(p_key, p_value);
}

void CocoaSDK::remove_context(const String &p_key) {
	objc_wrapper::remove_context_objc(p_key);
}

void CocoaSDK::set_tag(const String &p_key, const String &p_value) {
	objc_wrapper::set_tag_objc(p_key, p_value);
}

void CocoaSDK::remove_tag(const String &p_key) {
	objc_wrapper::remove_tag_objc(p_key);
}

void CocoaSDK::set_user(const Ref<SentryUser> &p_user) {
	if (p_user.is_valid()) {
		objc_wrapper::set_user_objc(
				p_user->get_id(),
				p_user->get_username(),
				p_user->get_email(),
				p_user->get_ip_address());
	} else {
		objc_wrapper::remove_user_objc();
	}
}

void CocoaSDK::remove_user() {
	objc_wrapper::remove_user_objc();
}

void CocoaSDK::add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
		const String &p_type, const Dictionary &p_data) {
	objc_wrapper::add_breadcrumb_objc(p_message, p_category, p_level, p_type);
}

String CocoaSDK::capture_message(const String &p_message, Level p_level) {
	return objc_wrapper::capture_message_objc(p_message, p_level);
}

String CocoaSDK::get_last_event_id() {
	return objc_wrapper::get_last_event_id_objc();
}

Ref<SentryEvent> CocoaSDK::create_event() {
	// TODO: Implement
	return nullptr;
}

String CocoaSDK::capture_event(const Ref<SentryEvent> &p_event) {
	// TODO: Implement
	return "";
}

void CocoaSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	// TODO: Implement
}

void CocoaSDK::initialize(const PackedStringArray &p_global_attachments) {
	String dsn = SentryOptions::get_singleton()->get_dsn();
	objc_wrapper::initialize_sentry_sdk(dsn, true);
	initialized = true;
}

CocoaSDK::~CocoaSDK() {
	if (initialized) {
		objc_wrapper::shutdown_sentry_sdk();
	}
}

} //namespace sentry
