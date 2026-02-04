#include "javascript_log.h"

#include "javascript_string_names.h"
#include "javascript_util.h"

#include <godot_cpp/classes/java_script_object.hpp>

namespace sentry::javascript {

LogLevel JavaScriptLog::get_level() const {
	ERR_FAIL_COND_V(js_obj.is_null(), LOG_LEVEL_INFO);

	String level_str = js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(level));

	// TODO: extract
	if (level_str == "trace") {
		return LOG_LEVEL_TRACE;
	} else if (level_str == "debug") {
		return LOG_LEVEL_DEBUG;
	} else if (level_str == "info") {
		return LOG_LEVEL_INFO;
	} else if (level_str == "warn") {
		return LOG_LEVEL_WARN;
	} else if (level_str == "error") {
		return LOG_LEVEL_ERROR;
	} else if (level_str == "fatal") {
		return LOG_LEVEL_FATAL;
	}

	return LOG_LEVEL_INFO;
}

void JavaScriptLog::set_level(LogLevel p_level) {
	ERR_FAIL_COND(js_obj.is_null());

	// TODO: extract
	String level_str;
	switch (p_level) {
		case LOG_LEVEL_TRACE:
			level_str = "trace";
			break;
		case LOG_LEVEL_DEBUG:
			level_str = "debug";
			break;
		case LOG_LEVEL_INFO:
			level_str = "info";
			break;
		case LOG_LEVEL_WARN:
			level_str = "warn";
			break;
		case LOG_LEVEL_ERROR:
			level_str = "error";
			break;
		case LOG_LEVEL_FATAL:
			level_str = "fatal";
			break;
		default:
			level_str = "info";
			break;
	}

	js_obj->set(JAVASCRIPT_SN(level), level_str);
}

String JavaScriptLog::get_body() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(message));
}

void JavaScriptLog::set_body(const String &p_body) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(message), p_body);
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

JavaScriptLog::JavaScriptLog(const Ref<RefCounted> &p_js_log_object) {
	js_obj = p_js_log_object;
}

JavaScriptLog::~JavaScriptLog() {
}

} // namespace sentry::javascript
