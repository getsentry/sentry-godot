#include "native_feedback.h"

#include "sentry/native/native_util.h"

namespace sentry::native {

void NativeFeedback::set_name(const String &p_name) {
	sentry_value_set_or_remove_string_by_key(native_feedback, "name", p_name);
}

String NativeFeedback::get_name() const {
	sentry_value_t name = sentry_value_get_by_key(native_feedback, "name");
	return String::utf8(sentry_value_as_string(name));
}

void NativeFeedback::set_contact_email(const String &p_contact_email) {
	sentry_value_set_or_remove_string_by_key(native_feedback, "contact_email", p_contact_email);
}

String NativeFeedback::get_contact_email() const {
	sentry_value_t contact_email = sentry_value_get_by_key(native_feedback, "contact_email");
	return String::utf8(sentry_value_as_string(contact_email));
}

void NativeFeedback::set_message(const String &p_message) {
	sentry_value_set_by_key(native_feedback, "message", sentry_value_new_string(p_message.utf8()));
}

String NativeFeedback::get_message() const {
	sentry_value_t message = sentry_value_get_by_key(native_feedback, "message");
	return String::utf8(sentry_value_as_string(message));
}

void NativeFeedback::set_associated_event_id(const String &p_associated_event_id) {
	sentry_value_set_or_remove_string_by_key(native_feedback, "associated_event_id", p_associated_event_id);
}

String NativeFeedback::get_associated_event_id() const {
	sentry_value_t associated_event_id = sentry_value_get_by_key(native_feedback, "associated_event_id");
	return String::utf8(sentry_value_as_string(associated_event_id));
}

NativeFeedback::NativeFeedback(sentry_value_t p_feedback) :
		native_feedback(p_feedback) {
	sentry_value_incref(p_feedback); // acquire ownership
}

NativeFeedback::~NativeFeedback() {
	sentry_value_decref(native_feedback); // release ownership
}

} //namespace sentry::native
