#include "javascript_feedback.h"

namespace sentry::javascript {

void JavaScriptFeedback::set_name(const String &p_name) {
	js_obj->set_or_remove_string_property("name", p_name.utf8());
}

String JavaScriptFeedback::get_name() const {
	return js_obj->get("name").as_string();
}

void JavaScriptFeedback::set_contact_email(const String &p_contact_email) {
	js_obj->set_or_remove_string_property("contact_email", p_contact_email.utf8());
}

String JavaScriptFeedback::get_contact_email() const {
	return js_obj->get("contact_email").as_string();
}

void JavaScriptFeedback::set_message(const String &p_message) {
	js_obj->set("message", p_message.utf8());
}

String JavaScriptFeedback::get_message() const {
	return js_obj->get("message").as_string();
}

void JavaScriptFeedback::set_associated_event_id(const String &p_associated_event_id) {
	js_obj->set_or_remove_string_property("associated_event_id", p_associated_event_id.utf8());
}

String JavaScriptFeedback::get_associated_event_id() const {
	return js_obj->get("associated_event_id").as_string();
}

JavaScriptFeedback::JavaScriptFeedback(const JSObjectPtr &p_js_feedback_object) :
		js_obj(p_js_feedback_object) {
}

} //namespace sentry::javascript
