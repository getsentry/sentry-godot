#include "android_feedback.h"

#include "android_string_names.h"

#include <godot_cpp/core/error_macros.hpp>

namespace sentry::android {

void AndroidFeedback::set_name(const String &p_name) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(feedbackSetName), handle, p_name);
}

String AndroidFeedback::get_name() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(feedbackGetName), handle);
}

void AndroidFeedback::set_contact_email(const String &p_contact_email) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(feedbackSetContactEmail), handle, p_contact_email);
}

String AndroidFeedback::get_contact_email() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(feedbackGetContactEmail), handle);
}

void AndroidFeedback::set_message(const String &p_message) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(feedbackSetMessage), handle, p_message);
}

String AndroidFeedback::get_message() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(feedbackGetMessage), handle);
}

void AndroidFeedback::set_associated_event_id(const String &p_associated_event_id) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(feedbackSetAssociatedEventId), handle, p_associated_event_id);
}

String AndroidFeedback::get_associated_event_id() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(feedbackGetAssociatedEventId), handle);
}

AndroidFeedback::AndroidFeedback(Object *p_android_plugin, int32_t p_feedback_handle) :
		android_plugin(p_android_plugin), handle(p_feedback_handle) {
	ERR_FAIL_NULL(p_android_plugin);
}

AndroidFeedback::~AndroidFeedback() {
	if (android_plugin) {
		android_plugin->call(ANDROID_SN(releaseFeedback), handle);
	}
}

} //namespace sentry::android
