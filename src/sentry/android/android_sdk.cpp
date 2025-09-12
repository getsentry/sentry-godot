#include "android_sdk.h"

#include "android_breadcrumb.h"
#include "android_event.h"
#include "android_string_names.h"
#include "sentry/common_defs.h"
#include "sentry/processing/process_event.h"
#include "sentry/sentry_attachment.h"
#include "sentry/util/print.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace sentry::android {

void SentryAndroidBeforeSendHandler::_initialize(Object *p_android_plugin) {
	android_plugin = p_android_plugin;
}

void SentryAndroidBeforeSendHandler::_before_send(int32_t p_event_handle) {
	sentry::util::print_debug("handling before_send: ", p_event_handle);

	Ref<AndroidEvent> event_obj = memnew(AndroidEvent(android_plugin, p_event_handle));
	event_obj->set_as_borrowed();

	Ref<AndroidEvent> processed = sentry::process_event(event_obj);

	if (processed.is_null()) {
		// Discard event.
		android_plugin->call(ANDROID_SN(releaseEvent), p_event_handle);
	}
}

void SentryAndroidBeforeSendHandler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("before_send"), &SentryAndroidBeforeSendHandler::_before_send);
}

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

Ref<SentryBreadcrumb> AndroidSDK::create_breadcrumb() {
	ERR_FAIL_NULL_V(android_plugin, nullptr);
	int32_t handle = android_plugin->call(ANDROID_SN(createBreadcrumb));
	Ref<AndroidBreadcrumb> crumb = memnew(AndroidBreadcrumb(android_plugin, handle));
	return crumb;
}

void AndroidSDK::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ERR_FAIL_NULL(android_plugin);
	Ref<AndroidBreadcrumb> crumb = p_breadcrumb;
	ERR_FAIL_COND(crumb.is_null());
	android_plugin->call(ANDROID_SN(addBreadcrumb), crumb->get_handle());
}

String AndroidSDK::capture_message(const String &p_message, Level p_level) {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(captureMessage), p_message, p_level);
}

String AndroidSDK::get_last_event_id() {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(getLastEventId));
}

Ref<SentryEvent> AndroidSDK::create_event() {
	ERR_FAIL_NULL_V(android_plugin, nullptr);
	int32_t event_handle = android_plugin->call(ANDROID_SN(createEvent));
	Ref<AndroidEvent> event = memnew(AndroidEvent(android_plugin, event_handle));
	return event;
}

String AndroidSDK::capture_event(const Ref<SentryEvent> &p_event) {
	ERR_FAIL_NULL_V(android_plugin, String());
	ERR_FAIL_COND_V(p_event.is_null(), String());
	Ref<AndroidEvent> android_event = p_event;
	ERR_FAIL_COND_V(android_event.is_null(), String());
	int32_t handle = android_event->get_handle();
	android_plugin->call(ANDROID_SN(captureEvent), handle);
	return android_event->get_id();
}

void AndroidSDK::add_attachment(const Ref<SentryAttachment> &p_attachment) {
	ERR_FAIL_COND(p_attachment.is_null());

	if (p_attachment->get_path().is_empty()) {
		sentry::util::print_debug("attaching bytes with filename: ", p_attachment->get_filename());
		android_plugin->call(ANDROID_SN(addBytesAttachment),
				p_attachment->get_bytes(),
				p_attachment->get_filename(),
				p_attachment->get_content_type(),
				String());
	} else {
		String absolute_path = ProjectSettings::get_singleton()->globalize_path(p_attachment->get_path());
		sentry::util::print_debug("attaching file: ", absolute_path);
		android_plugin->call(ANDROID_SN(addFileAttachment),
				absolute_path,
				p_attachment->get_filename(),
				p_attachment->get_content_type(),
				String());
	}
}

void AndroidSDK::init(const PackedStringArray &p_global_attachments, const Callable &p_configuration_callback) {
	ERR_FAIL_NULL(android_plugin);

	if (p_configuration_callback.is_valid()) {
		p_configuration_callback.call(SentryOptions::get_singleton());
	}

	for (const String &path : p_global_attachments) {
		bool is_view_hierarchy = path.ends_with(SENTRY_VIEW_HIERARCHY_FN);
		android_plugin->call(ANDROID_SN(addFileAttachment),
				path,
				String(), // filename
				is_view_hierarchy ? "application/json" : String(),
				is_view_hierarchy ? "event.view_hierarchy" : String());
	}

	android_plugin->call(ANDROID_SN(init),
			before_send_handler->get_instance_id(),
			SentryOptions::get_singleton()->get_dsn(),
			SentryOptions::get_singleton()->is_debug_enabled(),
			SentryOptions::get_singleton()->get_release(),
			SentryOptions::get_singleton()->get_dist(),
			SentryOptions::get_singleton()->get_environment(),
			SentryOptions::get_singleton()->get_sample_rate(),
			SentryOptions::get_singleton()->get_max_breadcrumbs());
}

void AndroidSDK::close() {
	if (android_plugin != nullptr) {
		android_plugin->call(ANDROID_SN(close));
	}
}

bool AndroidSDK::is_enabled() const {
	return android_plugin && android_plugin->call(ANDROID_SN(isEnabled));
}

AndroidSDK::AndroidSDK() {
	AndroidStringNames::create_singleton();

	android_plugin = Engine::get_singleton()->get_singleton("SentryAndroidGodotPlugin");
	ERR_FAIL_NULL_MSG(android_plugin, "Sentry: Unable to locate SentryAndroidGodotPlugin singleton.");

	before_send_handler = memnew(SentryAndroidBeforeSendHandler);
	before_send_handler->_initialize(android_plugin);
}

AndroidSDK::~AndroidSDK() {
	if (is_enabled()) {
		close();
	}

	AndroidStringNames::destroy_singleton();
	if (before_send_handler != nullptr) {
		memdelete(before_send_handler);
	}
}

} //namespace sentry::android
