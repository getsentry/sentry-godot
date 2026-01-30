#include "javascript_event.h"

#include "javascript_string_names.h"
#include "javascript_util.h"

#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/classes/java_script_object.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>

namespace {

// Returns an object property from a JS object, creating/assigning if missing.
Ref<JavaScriptObject> js_obj_get_or_create_object_property(const Ref<JavaScriptObject> &p_object, const StringName &p_property) {
	Ref<JavaScriptObject> prop_obj = p_object->get(p_property);
	if (prop_obj.is_null()) {
		prop_obj = JavaScriptBridge::get_singleton()->create_object(JAVASCRIPT_SN(Object));
		p_object->set(p_property, prop_obj);
	}
	return prop_obj;
}

// Returns an array property from a JS object, creating/assigning if missing.
Ref<JavaScriptObject> js_obj_get_or_create_array_property(const Ref<JavaScriptObject> &p_object, const StringName &p_property) {
	Ref<JavaScriptObject> prop_obj = p_object->get(p_property);
	if (prop_obj.is_null()) {
		prop_obj = JavaScriptBridge::get_singleton()->create_object(JAVASCRIPT_SN(Array));
		p_object->set(p_property, prop_obj);
	}
	return prop_obj;
}

} // unnamed namespace

namespace sentry::javascript {

String JavaScriptEvent::get_id() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(event_id));
}

void JavaScriptEvent::set_message(const String &p_message) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(message), p_message);
}

String JavaScriptEvent::get_message() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(message));
}

void JavaScriptEvent::set_timestamp(const Ref<SentryTimestamp> &p_timestamp) {
	ERR_FAIL_COND(js_obj.is_null());
	if (p_timestamp.is_valid()) {
		js_object_set_double(js_obj, JAVASCRIPT_SN(timestamp), p_timestamp->to_unix_time());
	} else {
		js_obj->set(JAVASCRIPT_SN(timestamp), Variant());
	}
}

Ref<SentryTimestamp> JavaScriptEvent::get_timestamp() const {
	ERR_FAIL_COND_V(js_obj.is_null(), Ref<SentryTimestamp>());
	double unix_time = js_object_get_double(js_obj, JAVASCRIPT_SN(timestamp), 0.0);
	if (unix_time == 0.0) {
		return Ref<SentryTimestamp>();
	}
	return SentryTimestamp::from_unix_time(unix_time);
}

String JavaScriptEvent::get_platform() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(platform));
}

void JavaScriptEvent::set_level(sentry::Level p_level) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(level), level_as_string(p_level));
}

sentry::Level JavaScriptEvent::get_level() const {
	ERR_FAIL_COND_V(js_obj.is_null(), sentry::Level::LEVEL_INFO);
	String level_str = js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(level));
	return sentry::level_from_string(level_str, sentry::Level::LEVEL_ERROR);
}

void JavaScriptEvent::set_logger(const String &p_logger) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(logger), p_logger);
}

String JavaScriptEvent::get_logger() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(logger));
}

void JavaScriptEvent::set_release(const String &p_release) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(release), p_release);
}

String JavaScriptEvent::get_release() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(release));
}

void JavaScriptEvent::set_dist(const String &p_dist) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(dist), p_dist);
}

String JavaScriptEvent::get_dist() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(dist));
}

void JavaScriptEvent::set_environment(const String &p_environment) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set(JAVASCRIPT_SN(environment), p_environment);
}

String JavaScriptEvent::get_environment() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(environment));
}

void JavaScriptEvent::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(js_obj.is_null());
	Ref<JavaScriptObject> tags_obj = js_obj_get_or_create_object_property(js_obj, JAVASCRIPT_SN(tags));
	tags_obj->set(p_key, p_value);
}

void JavaScriptEvent::remove_tag(const String &p_key) {
	ERR_FAIL_COND(js_obj.is_null());
	Ref<JavaScriptObject> tags_obj = js_obj->get(JAVASCRIPT_SN(tags));
	if (tags_obj.is_valid()) {
		js_delete_property(tags_obj, p_key);
	}
}

String JavaScriptEvent::get_tag(const String &p_key) {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	Ref<JavaScriptObject> tags_obj = js_obj->get(JAVASCRIPT_SN(tags));
	if (tags_obj.is_valid()) {
		return js_object_get_property_as_string(tags_obj, p_key);
	}
	return String();
}

void JavaScriptEvent::merge_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(js_obj.is_null());
	Ref<JavaScriptObject> contexts_obj = js_obj_get_or_create_object_property(js_obj, JAVASCRIPT_SN(contexts));
	Ref<JavaScriptObject> context_obj = js_obj_get_or_create_object_property(contexts_obj, p_key);
	js_merge_json_into_object(context_obj, JSON::stringify(p_value));
}

void JavaScriptEvent::add_exception(const Exception &p_exception) {
	ERR_FAIL_COND(js_obj.is_null());

	Ref<JavaScriptObject> exception_obj = js_obj_get_or_create_object_property(js_obj, JAVASCRIPT_SN(exception));
	Ref<JavaScriptObject> values_arr = js_obj_get_or_create_array_property(exception_obj, JAVASCRIPT_SN(values));

	js_push_json_to_array(values_arr, p_exception.to_json());
}

int JavaScriptEvent::get_exception_count() const {
	ERR_FAIL_COND_V(js_obj.is_null(), 0);
	Ref<JavaScriptObject> exception_obj = js_obj->get(JAVASCRIPT_SN(exception));
	if (exception_obj.is_valid()) {
		Ref<JavaScriptObject> values_arr = exception_obj->get(JAVASCRIPT_SN(values));
		if (values_arr.is_valid()) {
			return values_arr->get(JAVASCRIPT_SN(length));
		}
	}
	return 0;
}

void JavaScriptEvent::set_exception_value(int p_index, const String &p_value) {
	ERR_FAIL_COND(js_obj.is_null());

	Ref<JavaScriptObject> exception_obj = js_obj->get(JAVASCRIPT_SN(exception));
	if (exception_obj.is_null()) {
		WARN_PRINT("Sentry: Exception data not found.");
		return;
	}

	Ref<JavaScriptObject> values_arr = exception_obj->get(JAVASCRIPT_SN(values));
	if (values_arr.is_null()) {
		WARN_PRINT("Sentry: Exception values not found.");
		return;
	}

	int length = values_arr->get(JAVASCRIPT_SN(length));
	if (p_index < 0 || p_index >= length) {
		WARN_PRINT("Sentry: Exception with index " + itos(p_index) + " not found.");
		return;
	}

	Ref<JavaScriptObject> exc_obj = values_arr->get(String::num_int64(p_index));
	if (exc_obj.is_null()) {
		WARN_PRINT("Sentry: Expected exception object.");
		return;
	}

	exc_obj->set(JAVASCRIPT_SN(value), p_value);
}

String JavaScriptEvent::get_exception_value(int p_index) const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());

	Ref<JavaScriptObject> exception_obj = js_obj->get(JAVASCRIPT_SN(exception));
	if (exception_obj.is_null()) {
		return String();
	}

	Ref<JavaScriptObject> values_arr = exception_obj->get(JAVASCRIPT_SN(values));
	if (values_arr.is_null()) {
		return String();
	}

	int length = values_arr->get(JAVASCRIPT_SN(length));
	if (p_index < 0 || p_index >= length) {
		return String();
	}

	Ref<JavaScriptObject> exc_obj = values_arr->get(String::num_int64(p_index));
	if (exc_obj.is_null()) {
		return String();
	}

	return js_object_get_property_as_string(exc_obj, JAVASCRIPT_SN(value));
}

bool JavaScriptEvent::is_crash() const {
	return false;
}

String JavaScriptEvent::to_json() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_to_json(js_obj);
}

JavaScriptEvent::JavaScriptEvent(const Ref<RefCounted> &p_js_event_object) {
	js_obj = p_js_event_object;
}

JavaScriptEvent::JavaScriptEvent() {
	js_obj = JavaScriptBridge::get_singleton()->create_object(JAVASCRIPT_SN(Object));

	// Capture current timestamp
	js_object_set_double(js_obj, JAVASCRIPT_SN(timestamp),
			Time::get_singleton()->get_unix_time_from_system());
}

JavaScriptEvent::~JavaScriptEvent() {
}

} // namespace sentry::javascript
