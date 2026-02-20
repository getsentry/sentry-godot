#include "javascript_log.h"

namespace sentry::javascript {

LogLevel JavaScriptLog::get_level() const {
	ERR_FAIL_COND_V(!js_obj, LOG_LEVEL_INFO);

	String level_str = js_obj->get("level").as_string();
	return sentry::log_level_from_string(level_str, LOG_LEVEL_INFO);
}

void JavaScriptLog::set_level(LogLevel p_level) {
	ERR_FAIL_COND(!js_obj);

	js_obj->set("level", sentry::log_level_as_cstring(p_level));
}

String JavaScriptLog::get_body() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("message").as_string();
}

void JavaScriptLog::set_body(const String &p_body) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set_or_remove_string_property("message", p_body.utf8());
}

Variant JavaScriptLog::get_attribute(const String &p_name) const {
	ERR_FAIL_COND_V(!js_obj, Variant());

	JSObjectPtr attributes_obj = js_obj->get("attributes").as_object();
	if (!attributes_obj) {
		return Variant();
	}

	JSValue attr_val = attributes_obj->get(p_name.utf8());

	// Attributes can be stored either as a typed object { value, type } or as a raw primitive.
	JSObjectPtr attr_obj = attr_val.as_object();
	if (attr_obj) {
		// Typed attribute - return the underlying value.
		return attr_obj->get_as_variant("value");
	}

	// Raw primitive value.
	switch (attr_val.get_type()) {
		case JSValueType::BOOL: {
			return attr_val.as_bool();
		} break;
		case JSValueType::INT: {
			return attr_val.as_int();
		} break;
		case JSValueType::DOUBLE: {
			return attr_val.as_double();
		} break;
		case JSValueType::STRING: {
			return attr_val.as_string();
		} break;
		default: {
			return Variant();
		}
	}
}

void JavaScriptLog::set_attribute(const String &p_name, const Variant &p_value) {
	ERR_FAIL_COND(!js_obj);

	JSObjectPtr attr_obj = js_obj->get_or_create_object_property("attributes");
	switch (p_value.get_type()) {
		case Variant::Type::BOOL: {
			attr_obj->set(p_name.utf8(), p_value.operator bool());
		} break;
		case Variant::Type::INT: {
			attr_obj->set(p_name.utf8(), p_value.operator int64_t());
		} break;
		case Variant::Type::FLOAT: {
			attr_obj->set(p_name.utf8(), p_value.operator double());
		} break;
		case Variant::Type::STRING: {
			attr_obj->set(p_name.utf8(), p_value.operator String().utf8());
		} break;
		default: {
			attr_obj->set(p_name.utf8(), p_value.stringify().utf8());
		}
	}
}

void JavaScriptLog::add_attributes(const Dictionary &p_attributes) {
	ERR_FAIL_COND(!js_obj);

	Array keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		set_attribute(key, p_attributes[key]);
	}
}

void JavaScriptLog::remove_attribute(const String &p_name) {
	ERR_FAIL_COND(!js_obj);

	JSObjectPtr attr_obj = js_obj->get("attributes").as_object();
	if (attr_obj) {
		attr_obj->delete_property(p_name.utf8());
	}
}

JavaScriptLog::JavaScriptLog() {
	ERR_PRINT("This constructor is not intended for runtime use.");
}

JavaScriptLog::JavaScriptLog(const JSObjectPtr &p_js_log_object) {
	js_obj = p_js_log_object;
}

JavaScriptLog::~JavaScriptLog() {
}

} // namespace sentry::javascript
