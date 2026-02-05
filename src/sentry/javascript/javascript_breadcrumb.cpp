#include "javascript_breadcrumb.h"

#include "javascript_string_names.h"
#include "javascript_util.h"

#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/classes/java_script_object.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>

namespace sentry::javascript {

void JavaScriptBreadcrumb::set_message(const String &p_message) {
	ERR_FAIL_COND(js_obj.is_null());
	js_object_set_or_remove_string_property(js_obj, JAVASCRIPT_SN(message), p_message);
}

String JavaScriptBreadcrumb::get_message() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(message));
}

void JavaScriptBreadcrumb::set_category(const String &p_category) {
	ERR_FAIL_COND(js_obj.is_null());
	js_object_set_or_remove_string_property(js_obj, JAVASCRIPT_SN(category), p_category);
}

String JavaScriptBreadcrumb::get_category() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(category));
}

void JavaScriptBreadcrumb::set_level(sentry::Level p_level) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(level), level_as_string(p_level));
}

sentry::Level JavaScriptBreadcrumb::get_level() const {
	ERR_FAIL_COND_V(js_obj.is_null(), sentry::Level::LEVEL_INFO);

	Variant value = js_obj->get(JAVASCRIPT_SN(level));
	if (value == Variant()) {
		return sentry::Level::LEVEL_INFO;
	}

	return sentry::level_from_string(value, sentry::Level::LEVEL_INFO);
}

void JavaScriptBreadcrumb::set_type(const String &p_type) {
	ERR_FAIL_COND(js_obj.is_null());
	js_object_set_or_remove_string_property(js_obj, JAVASCRIPT_SN(type), p_type);
}

String JavaScriptBreadcrumb::get_type() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(type));
}

void JavaScriptBreadcrumb::set_data(const Dictionary &p_data) {
	ERR_FAIL_COND(js_obj.is_null());
	if (!p_data.is_empty()) {
		js_delete_property(js_obj, JAVASCRIPT_SN(data));
		Ref<JavaScriptObject> data_obj = js_object_get_or_create_object_property(js_obj, JAVASCRIPT_SN(data));
		js_merge_json_into_object(data_obj, JSON::stringify(p_data));
	} else {
		js_delete_property(js_obj, JAVASCRIPT_SN(data));
	}
}

Ref<SentryTimestamp> JavaScriptBreadcrumb::get_timestamp() {
	ERR_FAIL_COND_V(js_obj.is_null(), Ref<SentryTimestamp>());
	double unix_time = js_object_get_double(js_obj, JAVASCRIPT_SN(timestamp), 0.0);
	if (unix_time == 0.0) {
		return Ref<SentryTimestamp>();
	}
	return SentryTimestamp::from_unix_time(unix_time);
}

JavaScriptBreadcrumb::JavaScriptBreadcrumb() {
	js_obj = JavaScriptBridge::get_singleton()->create_object(JAVASCRIPT_SN(Object));

	// Capture current timestamp
	js_object_set_double(js_obj, JAVASCRIPT_SN(timestamp),
			Time::get_singleton()->get_unix_time_from_system());
}

JavaScriptBreadcrumb::~JavaScriptBreadcrumb() {
}

} // namespace sentry::javascript
