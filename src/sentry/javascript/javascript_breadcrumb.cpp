#include "javascript_breadcrumb.h"

#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/classes/java_script_object.hpp>
#include <godot_cpp/classes/json.hpp>

namespace sentry::javascript {

// TODO: cache StringName instances

void JavaScriptBreadcrumb::set_message(const String &p_message) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("message", p_message);
}

String JavaScriptBreadcrumb::get_message() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_obj->get("message");
}

void JavaScriptBreadcrumb::set_category(const String &p_category) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("category", p_category);
}

String JavaScriptBreadcrumb::get_category() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_obj->get("category");
}

void JavaScriptBreadcrumb::set_level(sentry::Level p_level) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("level", level_as_string(p_level));
}

sentry::Level JavaScriptBreadcrumb::get_level() const {
	ERR_FAIL_COND_V(js_obj.is_null(), sentry::Level::LEVEL_INFO);
	String level_str = js_obj->get("level");
	if (level_str == "debug") {
		return sentry::Level::LEVEL_DEBUG;
	} else if (level_str == "info") {
		return sentry::Level::LEVEL_INFO;
	} else if (level_str == "warning") {
		return sentry::Level::LEVEL_WARNING;
	} else if (level_str == "error") {
		return sentry::Level::LEVEL_ERROR;
	} else if (level_str == "fatal") {
		return sentry::Level::LEVEL_FATAL;
	} else {
		return sentry::Level::LEVEL_INFO;
	}
}

void JavaScriptBreadcrumb::set_type(const String &p_type) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("type", p_type);
}

String JavaScriptBreadcrumb::get_type() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_obj->get("type");
}

void JavaScriptBreadcrumb::set_data(const Dictionary &p_data) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("data", p_data);
}

Ref<SentryTimestamp> JavaScriptBreadcrumb::get_timestamp() {
	ERR_FAIL_COND_V(js_obj.is_null(), Ref<SentryTimestamp>());
	Variant timestamp_var = js_obj->get("timestamp");
	if (timestamp_var.get_type() == Variant::NIL) {
		return Ref<SentryTimestamp>();
	}

	double unix_time = timestamp_var;
	return SentryTimestamp::from_unix_time(unix_time);
}

JavaScriptBreadcrumb::JavaScriptBreadcrumb() {
	js_obj = JavaScriptBridge::get_singleton()->create_object("Object");
}

JavaScriptBreadcrumb::~JavaScriptBreadcrumb() {
}

} // namespace sentry::javascript
