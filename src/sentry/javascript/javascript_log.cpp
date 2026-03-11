#include "javascript_log.h"

#include "sentry/javascript/javascript_util.h"

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
	return sentry_js_object_get_attribute(js_obj, p_name);
}

void JavaScriptLog::set_attribute(const String &p_name, const Variant &p_value) {
	sentry_js_object_set_attribute(js_obj, p_name, p_value);
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
	js_obj = js_bridge()->create("Object");
	ERR_PRINT("This constructor is not intended for runtime use.");
}

JavaScriptLog::JavaScriptLog(const JSObjectPtr &p_js_log_object) {
	js_obj = p_js_log_object;
	ERR_FAIL_COND(!js_obj);
}

JavaScriptLog::~JavaScriptLog() {
}

} // namespace sentry::javascript
