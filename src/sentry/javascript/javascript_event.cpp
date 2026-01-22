#include "javascript_event.h"

#include "javascript_util.h"

#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/classes/java_script_object.hpp>
#include <godot_cpp/classes/json.hpp>

namespace {

inline Ref<JavaScriptObject> js_obj_get_or_create_object_property(const Ref<JavaScriptObject> &p_object, const StringName &p_property) {
	Ref<JavaScriptObject> prop_obj = p_object->get(p_property);
	if (prop_obj.is_null()) {
		prop_obj = JavaScriptBridge::get_singleton()->create_object("Object");
		p_object->set(p_property, prop_obj);
	}
	return prop_obj;
}

inline Ref<JavaScriptObject> js_obj_get_or_create_array_property(const Ref<JavaScriptObject> &p_object, const StringName &p_property) {
	Ref<JavaScriptObject> prop_obj = p_object->get(p_property);
	if (prop_obj.is_null()) {
		prop_obj = JavaScriptBridge::get_singleton()->create_object("Array");
		p_object->set(p_property, prop_obj);
	}
	return prop_obj;
}

} // unnamed namespace

namespace sentry::javascript {

String JavaScriptEvent::get_id() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_obj->get("event_id");
}

void JavaScriptEvent::set_message(const String &p_message) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("message", p_message);
}

String JavaScriptEvent::get_message() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_obj->get("message");
}

void JavaScriptEvent::set_timestamp(const Ref<SentryTimestamp> &p_timestamp) {
	ERR_FAIL_COND(js_obj.is_null());
	if (p_timestamp.is_valid()) {
		js_obj->set("timestamp", p_timestamp->get_microseconds_since_unix_epoch() / 1000000.0);
	} else {
		js_obj->set("timestamp", Variant());
	}
}

Ref<SentryTimestamp> JavaScriptEvent::get_timestamp() const {
	ERR_FAIL_COND_V(js_obj.is_null(), Ref<SentryTimestamp>());
	Variant timestamp_var = js_obj->get("timestamp");
	if (timestamp_var.get_type() == Variant::NIL) {
		return Ref<SentryTimestamp>();
	}
	double unix_time = timestamp_var;
	return SentryTimestamp::from_unix_time(unix_time);
}

String JavaScriptEvent::get_platform() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_obj->get("platform");
}

void JavaScriptEvent::set_level(sentry::Level p_level) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("level", level_as_string(p_level));
}

sentry::Level JavaScriptEvent::get_level() const {
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
		return sentry::Level::LEVEL_ERROR;
	}
}

void JavaScriptEvent::set_logger(const String &p_logger) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("logger", p_logger);
}

String JavaScriptEvent::get_logger() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_obj->get("logger");
}

void JavaScriptEvent::set_release(const String &p_release) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("release", p_release);
}

String JavaScriptEvent::get_release() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_obj->get("release");
}

void JavaScriptEvent::set_dist(const String &p_dist) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("dist", p_dist);
}

String JavaScriptEvent::get_dist() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_obj->get("dist");
}

void JavaScriptEvent::set_environment(const String &p_environment) {
	ERR_FAIL_COND(js_obj.is_null());
	js_obj->set("environment", p_environment);
}

String JavaScriptEvent::get_environment() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_obj->get("environment");
}

void JavaScriptEvent::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(js_obj.is_null());
	Ref<JavaScriptObject> tags_obj = js_obj_get_or_create_object_property(js_obj, "tags");
	tags_obj->set(p_key, p_value);
}

void JavaScriptEvent::remove_tag(const String &p_key) {
	ERR_FAIL_COND(js_obj.is_null());
	Ref<JavaScriptObject> tags_obj = js_obj->get("tags");
	if (tags_obj.is_valid()) {
		js_delete_property(tags_obj, p_key);
	}
}

String JavaScriptEvent::get_tag(const String &p_key) {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	Ref<JavaScriptObject> tags_obj = js_obj->get("tags");
	if (tags_obj.is_valid()) {
		return tags_obj->get(p_key);
	}
	return String();
}

void JavaScriptEvent::merge_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(js_obj.is_null());
	WARN_PRINT("Not implemented");
}

void JavaScriptEvent::add_exception(const Exception &p_exception) {
	ERR_FAIL_COND(js_obj.is_null());

	// Build exception dictionary
	Dictionary exc_dict;
	exc_dict["type"] = p_exception.type;
	exc_dict["value"] = p_exception.value;

	if (!p_exception.frames.is_empty()) {
		Array frames_array;
		for (int i = 0; i < p_exception.frames.size(); i++) {
			const StackFrame &frame = p_exception.frames[i];
			Dictionary frame_dict;
			if (!frame.filename.is_empty()) {
				frame_dict["filename"] = frame.filename;
			}
			if (!frame.function.is_empty()) {
				frame_dict["function"] = frame.function;
			}
			if (frame.lineno >= 0) {
				frame_dict["lineno"] = frame.lineno;
			}
			frame_dict["in_app"] = frame.in_app;
			if (!frame.platform.is_empty()) {
				frame_dict["platform"] = frame.platform;
			}
			if (!frame.context_line.is_empty()) {
				frame_dict["context_line"] = frame.context_line;
			}
			if (!frame.pre_context.is_empty()) {
				frame_dict["pre_context"] = frame.pre_context;
			}
			if (!frame.post_context.is_empty()) {
				frame_dict["post_context"] = frame.post_context;
			}
			if (!frame.vars.is_empty()) {
				Dictionary vars_dict;
				for (int j = 0; j < frame.vars.size(); j++) {
					vars_dict[frame.vars[j].first] = frame.vars[j].second;
				}
				frame_dict["vars"] = vars_dict;
			}
			frames_array.push_back(frame_dict);
		}
		Dictionary stacktrace;
		stacktrace["frames"] = frames_array;
		exc_dict["stacktrace"] = stacktrace;
	}

	Ref<JavaScriptObject> exception_obj = js_obj_get_or_create_object_property(js_obj, "exception");
	Ref<JavaScriptObject> values_arr = js_obj_get_or_create_array_property(exception_obj, "values");

	js_push_json_to_array(values_arr, JSON::stringify(exc_dict));
}

int JavaScriptEvent::get_exception_count() const {
	ERR_FAIL_COND_V(js_obj.is_null(), 0);
	WARN_PRINT("Not implemented");
	return 0;
}

void JavaScriptEvent::set_exception_value(int p_index, const String &p_value) {
	ERR_FAIL_COND(js_obj.is_null());
	WARN_PRINT("Not implemented");
}

String JavaScriptEvent::get_exception_value(int p_index) const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	WARN_PRINT("Not implemented");
	return String();
}

bool JavaScriptEvent::is_crash() const {
	WARN_PRINT("Not implemented");
	return false;
}

String JavaScriptEvent::to_json() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	WARN_PRINT("Not implemented");
	return String();
}

JavaScriptEvent::JavaScriptEvent() {
	js_obj = JavaScriptBridge::get_singleton()->create_object("Object");
}

JavaScriptEvent::~JavaScriptEvent() {
}

} // namespace sentry::javascript
