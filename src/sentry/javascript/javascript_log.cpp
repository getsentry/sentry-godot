#include "javascript_log.h"

#include "javascript_string_names.h"
#include "javascript_util.h"

namespace sentry::javascript {

LogLevel JavaScriptLog::get_level() const {
	ERR_FAIL_COND_V(js_obj.is_null(), LOG_LEVEL_INFO);

	String level_str = js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(level));
	return sentry::log_level_from_string(level_str, LOG_LEVEL_INFO);
}

void JavaScriptLog::set_level(LogLevel p_level) {
	ERR_FAIL_COND(js_obj.is_null());

	js_obj->set(JAVASCRIPT_SN(level), sentry::log_level_as_string(p_level));
}

String JavaScriptLog::get_body() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(message));
}

void JavaScriptLog::set_body(const String &p_body) {
	ERR_FAIL_COND(js_obj.is_null());
	js_object_set_or_remove_string_property(js_obj, JAVASCRIPT_SN(message), p_body);
}

Variant JavaScriptLog::get_attribute(const String &p_name) const {
	ERR_FAIL_COND_V(js_obj.is_null(), Variant());

	Ref<JavaScriptObject> attributes_obj = js_obj->get(JAVASCRIPT_SN(attributes));
	if (attributes_obj.is_null()) {
		return Variant();
	}

	Variant attr_val = attributes_obj->get(p_name);

	// Attributes can be stored either as a typed object { value, type } or as a raw primitive.
	Ref<JavaScriptObject> attr_obj = attr_val;
	if (attr_obj.is_valid()) {
		// Typed attribute - return the underlying value.
		return attr_obj->get(JAVASCRIPT_SN(value));
	}

	// Raw primitive value.
	return attr_val;
}

void JavaScriptLog::set_attribute(const String &p_name, const Variant &p_value) {
	ERR_FAIL_COND(js_obj.is_null());

	Ref<JavaScriptObject> attributes_obj = js_object_get_or_create_object_property(js_obj, JAVASCRIPT_SN(attributes));
	attributes_obj->set(p_name, p_value);
}

void JavaScriptLog::add_attributes(const Dictionary &p_attributes) {
	ERR_FAIL_COND(js_obj.is_null());

	Array keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		set_attribute(key, p_attributes[key]);
	}
}

void JavaScriptLog::remove_attribute(const String &p_name) {
	ERR_FAIL_COND(js_obj.is_null());

	Ref<JavaScriptObject> attributes_obj = js_obj->get(JAVASCRIPT_SN(attributes));
	if (attributes_obj.is_valid()) {
		js_delete_property(attributes_obj, p_name);
	}
}

JavaScriptLog::JavaScriptLog() {
	ERR_PRINT("This constructor is not intended for runtime use.");
}

JavaScriptLog::JavaScriptLog(const Ref<RefCounted> &p_js_log_object) {
	js_obj = p_js_log_object;
}

JavaScriptLog::~JavaScriptLog() {
}

} // namespace sentry::javascript
