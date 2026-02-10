#include "javascript_event.h"

#include "javascript_string_names.h"
#include "javascript_util.h"
#include "sentry/util/json_writer.h"
#include "sentry/uuid.h"

#include <godot_cpp/classes/java_script_bridge.hpp>
#include <godot_cpp/classes/java_script_object.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>

namespace sentry::javascript {

namespace {

// Helper function to write stacktrace object with frames to a JSONWriter.
// Writes: "stacktrace": { "frames": [...] }
void write_stacktrace_json(sentry::util::JSONWriter &p_jw, const Vector<SentryEvent::StackFrame> &p_frames) {
	p_jw.key("stacktrace");
	p_jw.begin_object(); // stacktrace {
	p_jw.key("frames");
	p_jw.begin_array(); // frames [
	for (const SentryEvent::StackFrame &frame : p_frames) {
		p_jw.begin_object(); // frame {
		p_jw.kv_string("filename", frame.filename);
		p_jw.kv_string("function", frame.function);
		if (frame.lineno >= 0) {
			p_jw.kv_int("lineno", frame.lineno);
		}
		p_jw.kv_bool("in_app", frame.in_app);
		p_jw.kv_string("platform", frame.platform);
		if (!frame.context_line.is_empty()) {
			p_jw.kv_string("context_line", frame.context_line);
		}
		if (!frame.pre_context.is_empty()) {
			p_jw.kv_string_array("pre_context", frame.pre_context);
		}
		if (!frame.post_context.is_empty()) {
			p_jw.kv_string_array("post_context", frame.post_context);
		}
		if (!frame.vars.is_empty()) {
			p_jw.key("vars");
			p_jw.begin_object(); // vars {
			for (int j = 0; j < frame.vars.size(); j++) {
				p_jw.kv_variant(frame.vars[j].first, frame.vars[j].second);
			}
			p_jw.end_object(); // } vars
		}
		p_jw.end_object(); // } frame
	}
	p_jw.end_array(); // ] frames
	p_jw.end_object(); // } stacktrace
}

} // namespace

String JavaScriptEvent::get_id() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(event_id));
}

void JavaScriptEvent::set_message(const String &p_message) {
	ERR_FAIL_COND(js_obj.is_null());
	js_object_set_or_remove_string_property(js_obj, JAVASCRIPT_SN(message), p_message);
}

String JavaScriptEvent::get_message() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());

	Variant message_value = js_obj->get(JAVASCRIPT_SN(message));

	if (message_value == Variant()) {
		return String();
	}

	if (message_value.get_type() == Variant::STRING) {
		return message_value.operator String();
	}

	Ref<JavaScriptObject> jso = message_value;
	if (jso.is_valid()) {
		return js_object_get_property_as_string(jso, JAVASCRIPT_SN(formatted));
	}

	return String();
}

void JavaScriptEvent::set_timestamp(const Ref<SentryTimestamp> &p_timestamp) {
	ERR_FAIL_COND(js_obj.is_null());
	if (p_timestamp.is_valid()) {
		js_object_set_double(js_obj, JAVASCRIPT_SN(timestamp), p_timestamp->to_unix_time());
	} else {
		js_delete_property(js_obj, JAVASCRIPT_SN(timestamp));
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
	ERR_FAIL_COND_V(js_obj.is_null(), sentry::Level::LEVEL_ERROR);
	String level_str = js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(level));
	return sentry::level_from_string(level_str, sentry::Level::LEVEL_ERROR);
}

void JavaScriptEvent::set_logger(const String &p_logger) {
	ERR_FAIL_COND(js_obj.is_null());
	js_object_set_or_remove_string_property(js_obj, JAVASCRIPT_SN(logger), p_logger);
}

String JavaScriptEvent::get_logger() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(logger));
}

void JavaScriptEvent::set_release(const String &p_release) {
	ERR_FAIL_COND(js_obj.is_null());
	js_object_set_or_remove_string_property(js_obj, JAVASCRIPT_SN(release), p_release);
}

String JavaScriptEvent::get_release() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(release));
}

void JavaScriptEvent::set_dist(const String &p_dist) {
	ERR_FAIL_COND(js_obj.is_null());
	js_object_set_or_remove_string_property(js_obj, JAVASCRIPT_SN(dist), p_dist);
}

String JavaScriptEvent::get_dist() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(dist));
}

void JavaScriptEvent::set_environment(const String &p_environment) {
	ERR_FAIL_COND(js_obj.is_null());
	js_object_set_or_remove_string_property(js_obj, JAVASCRIPT_SN(environment), p_environment);
}

String JavaScriptEvent::get_environment() const {
	ERR_FAIL_COND_V(js_obj.is_null(), String());
	return js_object_get_property_as_string(js_obj, JAVASCRIPT_SN(environment));
}

void JavaScriptEvent::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(js_obj.is_null());
	Ref<JavaScriptObject> tags_obj = js_object_get_or_create_object_property(js_obj, JAVASCRIPT_SN(tags));
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
	Ref<JavaScriptObject> contexts_obj = js_object_get_or_create_object_property(js_obj, JAVASCRIPT_SN(contexts));
	Ref<JavaScriptObject> context_obj = js_object_get_or_create_object_property(contexts_obj, p_key);
	js_merge_json_into_object(context_obj, JSON::stringify(p_value));
}

void JavaScriptEvent::add_exception(const Exception &p_exception) {
	ERR_FAIL_COND(js_obj.is_null());

	uint64_t thread_id = OS::get_singleton()->get_thread_caller_id();
	bool is_main = OS::get_singleton()->get_main_thread_id() == thread_id;

	// Create thread structure with stacktrace.
	if (!p_exception.frames.is_empty()) {
		sentry::util::JSONWriter jw;
		jw.begin_object(); // thread {
		jw.kv_int("id", thread_id);
		jw.kv_bool("main", is_main);
		jw.kv_bool("crashed", true);
		jw.kv_bool("current", true);
		write_stacktrace_json(jw, p_exception.frames);
		jw.end_object(); // } thread

		Ref<JavaScriptObject> threads_obj = js_object_get_or_create_object_property(js_obj, JAVASCRIPT_SN(threads));
		Ref<JavaScriptObject> threads_arr = js_object_get_or_create_array_property(threads_obj, JAVASCRIPT_SN(values));
		js_push_json_to_array(threads_arr, jw.get_string());
	}

	// Create exception with thread_id reference.
	// NOTE: stacktrace is attached to the thread object, not the exception.
	sentry::util::JSONWriter exc_jw;
	exc_jw.begin_object(); // exception {
	exc_jw.kv_string("type", p_exception.type);
	exc_jw.kv_string("value", p_exception.value);
	exc_jw.kv_int("thread_id", thread_id);
	exc_jw.end_object(); // } exception

	Ref<JavaScriptObject> exception_obj = js_object_get_or_create_object_property(js_obj, JAVASCRIPT_SN(exception));
	Ref<JavaScriptObject> values_arr = js_object_get_or_create_array_property(exception_obj, JAVASCRIPT_SN(values));
	js_push_json_to_array(values_arr, exc_jw.get_string());
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

	// Pre-generate event-id
	js_obj->set(JAVASCRIPT_SN(event_id), sentry::uuid::make_uuid_no_dashes());
}

JavaScriptEvent::~JavaScriptEvent() {
}

} // namespace sentry::javascript
