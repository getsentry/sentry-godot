#include "javascript_breadcrumb.h"

#include "javascript_string_names.h"
#include "javascript_util.h"

#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/classes/java_script_object.hpp>
#include <godot_cpp/classes/json.hpp>

namespace {

inline String js_obj_get_property_value_as_string(const Ref<JavaScriptObject> &p_obj, const String &p_property, const String &p_default = "") {
	Variant val = p_obj->get(p_property);
	if (val == Variant()) {
		return p_default;
	}
	return val;
}

} // unnamed namespace

namespace sentry::javascript {

void JavaScriptBreadcrumb::set_message(const String &p_message) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(message), p_message);
}

String JavaScriptBreadcrumb::get_message() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(message));
}

void JavaScriptBreadcrumb::set_category(const String &p_category) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(category), p_category);
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
	js_obj->set(JAVASCRIPT_SN(type), p_type);
}

String JavaScriptBreadcrumb::get_type() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(type));
}

void JavaScriptBreadcrumb::set_data(const Dictionary &p_data) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(data), p_data);
}

Ref<SentryTimestamp> JavaScriptBreadcrumb::get_timestamp() {
	ERR_FAIL_COND_V(js_obj.is_null(), Ref<SentryTimestamp>());
	Variant timestamp_var = js_obj->get(JAVASCRIPT_SN(timestamp));
	if (timestamp_var.get_type() == Variant::NIL) {
		return Ref<SentryTimestamp>();
	}

	double unix_time = timestamp_var;
	return SentryTimestamp::from_unix_time(unix_time);
}

JavaScriptBreadcrumb::JavaScriptBreadcrumb() {
	js_obj = JavaScriptBridge::get_singleton()->create_object(JAVASCRIPT_SN(Object));
}

JavaScriptBreadcrumb::~JavaScriptBreadcrumb() {
}

} // namespace sentry::javascript
