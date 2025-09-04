#include "native_event.h"

#include "sentry/level.h"
#include "sentry/native/native_util.h"

#include <sentry.h>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace {

inline void _sentry_value_set_or_remove_string_by_key(sentry_value_t value, const char *k, const String &v) {
	if (v.is_empty()) {
		sentry_value_remove_by_key(value, k);
	} else {
		sentry_value_set_by_key(value, k, sentry_value_new_string(v.utf8()));
	}
}

void sentry_event_merge_context(sentry_value_t p_event, const char *p_context_name, const Dictionary &p_context) {
	ERR_FAIL_COND(sentry_value_get_type(p_event) != SENTRY_VALUE_TYPE_OBJECT);
	ERR_FAIL_COND(p_context_name == nullptr || strlen(p_context_name) == 0);

	if (p_context.is_empty()) {
		return;
	}

	sentry_value_t contexts = sentry_value_get_by_key(p_event, "contexts");
	if (sentry_value_is_null(contexts)) {
		contexts = sentry_value_new_object();
		sentry_value_set_by_key(p_event, "contexts", contexts);
	}

	// Check if context exists and update or add it.
	sentry_value_t ctx = sentry_value_get_by_key(contexts, p_context_name);
	if (!sentry_value_is_null(ctx)) {
		// If context exists, update it with new values.
		const Array &updated_keys = p_context.keys();
		for (int i = 0; i < updated_keys.size(); i++) {
			const String &key = updated_keys[i];
			sentry_value_set_by_key(ctx, key.utf8(), sentry::native::variant_to_sentry_value(p_context[key]));
		}
	} else {
		// If context doesn't exist, add it.
		sentry_value_set_by_key(contexts, p_context_name, sentry::native::variant_to_sentry_value(p_context));
	}
}

} // unnamed namespace

namespace sentry::native {

String NativeEvent::get_id() const {
	sentry_value_t id = sentry_value_get_by_key(native_event, "event_id");
	return sentry_value_as_string(id);
}

void NativeEvent::set_message(const String &p_message) {
	if (p_message.is_empty()) {
		sentry_value_remove_by_key(native_event, "message");
	} else {
		sentry_value_t message = sentry_value_get_by_key(native_event, "message");
		if (sentry_value_is_null(message)) {
			message = sentry_value_new_object();
			sentry_value_set_by_key(native_event, "message", message);
		}
		sentry_value_set_by_key(message, "formatted", sentry_value_new_string(p_message.utf8()));
	}
}

String NativeEvent::get_message() const {
	sentry_value_t message = sentry_value_get_by_key(native_event, "message");
	if (sentry_value_is_null(message)) {
		return String();
	}
	sentry_value_t formatted = sentry_value_get_by_key(message, "formatted");
	return String::utf8(sentry_value_as_string(formatted));
}

void NativeEvent::set_timestamp(const Ref<SentryTimestamp> &p_timestamp) {
	if (p_timestamp.is_valid()) {
		_sentry_value_set_or_remove_string_by_key(native_event, "timestamp", p_timestamp->to_rfc3339());
	} else {
		sentry_value_remove_by_key(native_event, "timestamp");
	}
}

Ref<SentryTimestamp> NativeEvent::get_timestamp() const {
	sentry_value_t timestamp = sentry_value_get_by_key(native_event, "timestamp");
	return SentryTimestamp::parse_rfc3339_cstr(sentry_value_as_string(timestamp));
}

String NativeEvent::get_platform() const {
	sentry_value_t platform = sentry_value_get_by_key(native_event, "platform");
	return String::utf8(sentry_value_as_string(platform));
}

void NativeEvent::set_level(sentry::Level p_level) {
	sentry_value_set_by_key(native_event, "level",
			sentry_value_new_string(sentry::native::level_to_cstring(p_level)));
}

sentry::Level NativeEvent::get_level() const {
	sentry_value_t value = sentry_value_get_by_key(native_event, "level");
	if (sentry_value_is_null(value)) {
		// The "level" is not set for new events by default, and GDScript lacks optional types,
		// so we need a default return value. Given that Sentry categorizes events with an error level
		// by default, that is what we return.
		return sentry::Level::LEVEL_ERROR;
	}
	return sentry::native::cstring_to_level(sentry_value_as_string(value));
}

void NativeEvent::set_logger(const String &p_logger) {
	_sentry_value_set_or_remove_string_by_key(native_event, "logger", p_logger);
}

String NativeEvent::get_logger() const {
	sentry_value_t logger = sentry_value_get_by_key(native_event, "logger");
	return String::utf8(sentry_value_as_string(logger));
}

void NativeEvent::set_release(const String &p_release) {
	_sentry_value_set_or_remove_string_by_key(native_event, "release", p_release);
}

String NativeEvent::get_release() const {
	sentry_value_t release = sentry_value_get_by_key(native_event, "release");
	return String::utf8(sentry_value_as_string(release));
}

void NativeEvent::set_dist(const String &p_dist) {
	_sentry_value_set_or_remove_string_by_key(native_event, "dist", p_dist);
}

String NativeEvent::get_dist() const {
	sentry_value_t dist = sentry_value_get_by_key(native_event, "dist");
	return String::utf8(sentry_value_as_string(dist));
}

void NativeEvent::set_environment(const String &p_environment) {
	_sentry_value_set_or_remove_string_by_key(native_event, "environment", p_environment);
}

String NativeEvent::get_environment() const {
	sentry_value_t environment = sentry_value_get_by_key(native_event, "environment");
	return String::utf8(sentry_value_as_string(environment));
}

void NativeEvent::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set tag with an empty key.");
	sentry_value_t tags = sentry_value_get_by_key(native_event, "tags");
	if (sentry_value_is_null(tags)) {
		tags = sentry_value_new_object();
		sentry_value_set_by_key(native_event, "tags", tags);
	}
	sentry_value_set_by_key(tags, p_key.utf8(), sentry_value_new_string(p_value.utf8()));
}

void NativeEvent::remove_tag(const String &p_key) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't remove tag with an empty key.");
	sentry_value_t tags = sentry_value_get_by_key(native_event, "tags");
	if (!sentry_value_is_null(tags)) {
		sentry_value_remove_by_key(tags, p_key.utf8());
	}
}

String NativeEvent::get_tag(const String &p_key) {
	ERR_FAIL_COND_V_MSG(p_key.is_empty(), String(), "Sentry: Can't get tag with an empty key.");
	sentry_value_t tags = sentry_value_get_by_key(native_event, "tags");
	if (!sentry_value_is_null(tags)) {
		sentry_value_t value = sentry_value_get_by_key(tags, p_key.utf8());
		return String::utf8(sentry_value_as_string(value));
	}
	return String();
}

void NativeEvent::merge_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't merge context with an empty key.");
	sentry_event_merge_context(native_event, p_key.utf8(), p_value);
}

void NativeEvent::add_exception(const Exception &p_exception) {
	sentry_value_t frames = sentry_value_new_list();

	for (const StackFrame &frame : p_exception.frames) {
		sentry_value_t sentry_frame = sentry_value_new_object();
		sentry_value_set_by_key(sentry_frame, "filename", sentry_value_new_string(frame.filename.utf8()));
		sentry_value_set_by_key(sentry_frame, "function", sentry_value_new_string(frame.function.utf8()));
		sentry_value_set_by_key(sentry_frame, "lineno", sentry_value_new_int32(frame.lineno));
		sentry_value_set_by_key(sentry_frame, "in_app", sentry_value_new_bool(frame.in_app));
		sentry_value_set_by_key(sentry_frame, "platform", sentry_value_new_string(frame.platform.utf8()));
		if (!frame.context_line.is_empty()) {
			sentry_value_set_by_key(sentry_frame, "context_line", sentry_value_new_string(frame.context_line.utf8()));
			sentry_value_set_by_key(sentry_frame, "pre_context", sentry::native::strings_to_sentry_list(frame.pre_context));
			sentry_value_set_by_key(sentry_frame, "post_context", sentry::native::strings_to_sentry_list(frame.post_context));
		}
		if (frame.vars.size() > 0) {
			sentry_value_t vars = sentry_value_new_object();
			sentry_value_set_by_key(sentry_frame, "vars", vars);
			for (auto pair : frame.vars) {
				sentry_value_set_by_key(vars, pair.first.utf8(), sentry::native::variant_to_sentry_value(pair.second));
			}
		}
		sentry_value_append(frames, sentry_frame);
	}

	sentry_value_t stack_trace = sentry_value_new_object();
	sentry_value_set_by_key(stack_trace, "frames", frames);

	uint64_t thread_id = godot::OS::get_singleton()->get_thread_caller_id();
	bool is_main = godot::OS::get_singleton()->get_main_thread_id() == thread_id;

	sentry_value_t thread = sentry_value_new_thread(thread_id, NULL);
	sentry_value_set_by_key(thread, "main", sentry_value_new_bool(is_main));
	// Set `crashed` to true to indicate that this thread is responsible for the event,
	// even if it's not a crash event.
	sentry_value_set_by_key(thread, "crashed", sentry_value_new_bool(true));
	sentry_value_set_by_key(thread, "current", sentry_value_new_bool(true));
	sentry_value_set_by_key(thread, "stacktrace", stack_trace);

	sentry_value_t native_exception = sentry_value_new_exception(
			p_exception.type.utf8(), p_exception.value.utf8());
	sentry_value_set_by_key(native_exception, "stacktrace", stack_trace);
	sentry_value_set_by_key(native_exception, "thread_id", sentry_value_new_uint64(thread_id));

	sentry_event_add_thread(native_event, thread);
	sentry_event_add_exception(native_event, native_exception);
}

bool NativeEvent::is_crash() const {
	return _is_crash;
}

String NativeEvent::to_json() const {
	char *json_value = sentry_value_to_json(native_event);
	String json_str = String::utf8(json_value);
	sentry_free(json_value);
	return json_str;
}

NativeEvent::NativeEvent(sentry_value_t p_native_event, bool p_is_crash) :
		_is_crash(p_is_crash) {
	if (sentry_value_refcount(p_native_event) > 0) {
		sentry_value_incref(p_native_event); // acquire ownership
		native_event = p_native_event;
	} else {
		// Shouldn't happen in healthy code.
		native_event = sentry_value_new_event();
		ERR_PRINT("Sentry: Internal error: Event refcount is zero.");
	}
}

NativeEvent::NativeEvent() {
	native_event = sentry_value_new_event();
}

NativeEvent::~NativeEvent() {
	sentry_value_decref(native_event); // release ownership
}

} //namespace sentry::native
