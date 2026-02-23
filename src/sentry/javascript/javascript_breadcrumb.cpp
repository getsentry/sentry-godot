#include "javascript_breadcrumb.h"

#include "sentry/logging/print.h"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>

namespace sentry::javascript {

void JavaScriptBreadcrumb::set_message(const String &p_message) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set_or_remove_string_property("message", p_message.utf8());
}

String JavaScriptBreadcrumb::get_message() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("message").as_string();
}

void JavaScriptBreadcrumb::set_category(const String &p_category) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set_or_remove_string_property("category", p_category.utf8());
}

String JavaScriptBreadcrumb::get_category() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("category").as_string();
}

void JavaScriptBreadcrumb::set_level(sentry::Level p_level) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set("level", level_as_cstring(p_level));
}

sentry::Level JavaScriptBreadcrumb::get_level() const {
	ERR_FAIL_COND_V(!js_obj, sentry::Level::LEVEL_INFO);

	JSValue value = js_obj->get("level");
	switch (value.get_type()) {
		case JSValueType::NIL: {
			return sentry::Level::LEVEL_INFO;
		} break;
		case JSValueType::STRING: {
			return level_from_string(value.as_string(), sentry::Level::LEVEL_INFO);
		} break;
		default: {
			sentry::logging::print_error("Breadcrumb has unexpected level value.");
			return sentry::Level::LEVEL_INFO;
		} break;
	}
}

void JavaScriptBreadcrumb::set_type(const String &p_type) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set_or_remove_string_property("type", p_type.utf8());
}

String JavaScriptBreadcrumb::get_type() const {
	ERR_FAIL_COND_V(!js_obj, "");
	return js_obj->get("type").as_string();
}

void JavaScriptBreadcrumb::set_data(const Dictionary &p_data) {
	ERR_FAIL_COND(!js_obj);

	if (!p_data.is_empty()) {
		JSObjectPtr data_obj = JSObject::create("Object");
		if (data_obj) {
			data_obj->merge_properties_from_json(JSON::stringify(p_data).utf8());
			js_obj->set("data", data_obj);
		}
	} else {
		js_obj->delete_property("data");
	}
}

Ref<SentryTimestamp> JavaScriptBreadcrumb::get_timestamp() {
	ERR_FAIL_COND_V(!js_obj, Ref<SentryTimestamp>());

	double unix_time = js_obj->get("timestamp").as_double();
	if (unix_time == 0.0) {
		return Ref<SentryTimestamp>();
	}
	return SentryTimestamp::from_unix_time(unix_time);
}

JavaScriptBreadcrumb::JavaScriptBreadcrumb() {
	js_obj = JSObject::create("Object");
	ERR_FAIL_COND(!js_obj);

	// Capture current timestamp
	js_obj->set("timestamp", Time::get_singleton()->get_unix_time_from_system());
}

JavaScriptBreadcrumb::~JavaScriptBreadcrumb() {
}

} // namespace sentry::javascript
