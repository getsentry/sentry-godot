#include "javascript_event.h"

#include "sentry/util/json_writer.h"
#include "sentry/uuid.h"

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
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("event_id").as_string();
}

void JavaScriptEvent::set_message(const String &p_message) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set_or_remove_string_property("message", p_message.utf8());
}

String JavaScriptEvent::get_message() const {
	ERR_FAIL_COND_V(!js_obj, String());

	JSValue message_value = js_obj->get("message");

	if (message_value.get_type() == JSValueType::NIL) {
		return String();
	}

	if (message_value.get_type() == JSValueType::STRING) {
		return message_value.as_string();
	}

	JSObjectPtr jso = message_value.as_object();
	if (jso) {
		return jso->get("formatted").as_string();
	}

	return String();
}

void JavaScriptEvent::set_timestamp(const Ref<SentryTimestamp> &p_timestamp) {
	ERR_FAIL_COND(!js_obj);

	if (p_timestamp.is_valid()) {
		js_obj->set("timestamp", p_timestamp->to_unix_time());
	} else {
		js_obj->delete_property("timestamp");
	}
}

Ref<SentryTimestamp> JavaScriptEvent::get_timestamp() const {
	ERR_FAIL_COND_V(!js_obj, Ref<SentryTimestamp>());

	double unix_time = js_obj->get("timestamp").as_double();
	if (unix_time == 0.0) {
		return Ref<SentryTimestamp>();
	}
	return SentryTimestamp::from_unix_time(unix_time);
}

String JavaScriptEvent::get_platform() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("platform").as_string();
}

void JavaScriptEvent::set_level(sentry::Level p_level) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set("level", level_as_cstring(p_level));
}

sentry::Level JavaScriptEvent::get_level() const {
	ERR_FAIL_COND_V(!js_obj, sentry::Level::LEVEL_ERROR);
	String level_str = js_obj->get("level").as_string();
	return sentry::level_from_string(level_str, sentry::Level::LEVEL_ERROR);
}

void JavaScriptEvent::set_logger(const String &p_logger) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set_or_remove_string_property("logger", p_logger.utf8());
}

String JavaScriptEvent::get_logger() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("logger").as_string();
}

void JavaScriptEvent::set_release(const String &p_release) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set_or_remove_string_property("release", p_release.utf8());
}

String JavaScriptEvent::get_release() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("release").as_string();
}

void JavaScriptEvent::set_dist(const String &p_dist) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set_or_remove_string_property("dist", p_dist.utf8());
}

String JavaScriptEvent::get_dist() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("dist").as_string();
}

void JavaScriptEvent::set_environment(const String &p_environment) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set_or_remove_string_property("environment", p_environment.utf8());
}

String JavaScriptEvent::get_environment() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("environment").as_string();
}

void JavaScriptEvent::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND(!js_obj);
	JSObjectPtr tags_obj = js_obj->get_or_create_object_property("tags");
	tags_obj->set(p_key.utf8(), p_value.utf8());
}

void JavaScriptEvent::remove_tag(const String &p_key) {
	ERR_FAIL_COND(!js_obj);
	JSObjectPtr tags_obj = js_obj->get_or_create_object_property("tags");
	if (tags_obj) {
		tags_obj->delete_property(p_key.utf8());
	}
}

String JavaScriptEvent::get_tag(const String &p_key) {
	ERR_FAIL_COND_V(!js_obj, String());
	JSObjectPtr tags_obj = js_obj->get_or_create_object_property("tags");
	if (tags_obj) {
		return tags_obj->get(p_key.utf8()).as_string();
	}
	return String();
}

void JavaScriptEvent::merge_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND(!js_obj);
	JSObjectPtr contexts_obj = js_obj->get_or_create_object_property("contexts");
	JSObjectPtr context_obj = contexts_obj->get_or_create_object_property(p_key.utf8());
	context_obj->merge_properties_from_json(JSON::stringify(p_value).utf8());
}

void JavaScriptEvent::add_exception(const Exception &p_exception) {
	ERR_FAIL_COND(!js_obj);

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

		JSObjectPtr threads_obj = js_obj->get_or_create_object_property("threads");
		JSObjectPtr threads_arr = threads_obj->get_or_create_array_property("values");
		threads_arr->push_element_from_json(jw.get_string().utf8());
	}

	// Create exception with thread_id reference.
	// NOTE: stacktrace is attached to the thread object, not the exception.
	sentry::util::JSONWriter exc_jw;
	exc_jw.begin_object(); // exception {
	exc_jw.kv_string("type", p_exception.type);
	exc_jw.kv_string("value", p_exception.value);
	exc_jw.kv_int("thread_id", thread_id);
	exc_jw.end_object(); // } exception

	JSObjectPtr exception_obj = js_obj->get_or_create_object_property("exception");
	JSObjectPtr values_arr = exception_obj->get_or_create_array_property("values");
	values_arr->push_element_from_json(exc_jw.get_string().utf8());
}

int JavaScriptEvent::get_exception_count() const {
	ERR_FAIL_COND_V(!js_obj, 0);
	JSObjectPtr exception_obj = js_obj->get("exception").as_object();
	if (exception_obj) {
		JSObjectPtr values_arr = exception_obj->get("values").as_object();
		if (values_arr) {
			return values_arr->get("length").as_int();
		}
	}
	return 0;
}

void JavaScriptEvent::set_exception_value(int p_index, const String &p_value) {
	ERR_FAIL_COND(!js_obj);

	JSObjectPtr exception_obj = js_obj->get("exception").as_object();
	if (!exception_obj) {
		WARN_PRINT("Sentry: Exception data not found.");
		return;
	}

	JSObjectPtr values_arr = exception_obj->get("values").as_object();
	if (!values_arr) {
		WARN_PRINT("Sentry: Exception values not found.");
		return;
	}

	int64_t length = values_arr->get("length").as_int();
	if (p_index < 0 || p_index >= length) {
		WARN_PRINT("Sentry: Exception with index " + itos(p_index) + " not found.");
		return;
	}

	JSObjectPtr exc_obj = values_arr->get(String::num_int64(p_index).ascii()).as_object();
	if (!exc_obj) {
		WARN_PRINT("Sentry: Expected exception object.");
		return;
	}

	exc_obj->set("value", p_value.utf8());
}

String JavaScriptEvent::get_exception_value(int p_index) const {
	ERR_FAIL_COND_V(!js_obj, String());

	JSObjectPtr exception_obj = js_obj->get("exception").as_object();
	if (!exception_obj) {
		return String();
	}

	JSObjectPtr values_arr = exception_obj->get("values").as_object();
	if (!values_arr) {
		return String();
	}

	int64_t length = values_arr->get("length").as_int();
	if (p_index < 0 || p_index >= length) {
		return String();
	}

	JSObjectPtr exc_obj = values_arr->get(String::num_int64(p_index).ascii()).as_object();
	if (!exc_obj) {
		return String();
	}

	return exc_obj->get("value").as_string();
}

bool JavaScriptEvent::is_crash() const {
	return false;
}

String JavaScriptEvent::to_json() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->to_json();
}

JavaScriptEvent::JavaScriptEvent(const JSObjectPtr &p_js_event_object) {
	js_obj = p_js_event_object;
}

JavaScriptEvent::JavaScriptEvent() {
	js_obj = JSObject::create("Object");

	// Capture current timestamp
	js_obj->set("timestamp", Time::get_singleton()->get_unix_time_from_system());

	// Pre-generate event-id
	js_obj->set("event_id", sentry::uuid::make_uuid_no_dashes().ascii());
}

JavaScriptEvent::~JavaScriptEvent() {
}

} // namespace sentry::javascript
