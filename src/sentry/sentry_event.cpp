#include "sentry_event.h"

#include "sentry/sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>
#include "sentry/util/json_writer.h"

#include <godot_cpp/classes/global_constants.hpp>

namespace sentry {

String SentryEvent::Exception::to_json() const {
	sentry::util::JSONWriter jw;
	jw.begin_object(); // exception {
	jw.kv_string("type", type);
	jw.kv_string("value", value);

	if (!frames.is_empty()) {
		jw.key("stacktrace");
		jw.begin_object(); // stacktrace {
		jw.key("frames");
		jw.begin_array(); // frames [
		for (int i = 0; i < frames.size(); i++) {
			const sentry::SentryEvent::StackFrame &frame = frames[i];
			jw.begin_object(); // frame {
			if (!frame.filename.is_empty()) {
				jw.kv_string("filename", frame.filename);
			}
			if (!frame.function.is_empty()) {
				jw.kv_string("function", frame.function);
			}
			if (frame.lineno >= 0) {
				jw.kv_int("lineno", frame.lineno);
			}
			jw.kv_bool("in_app", frame.in_app);
			if (!frame.platform.is_empty()) {
				jw.kv_string("platform", frame.platform);
			}
			if (!frame.context_line.is_empty()) {
				jw.kv_string("context_line", frame.context_line);
			}
			if (!frame.pre_context.is_empty()) {
				jw.kv_string_array("pre_context", frame.pre_context);
			}
			if (!frame.post_context.is_empty()) {
				jw.kv_string_array("post_context", frame.post_context);
			}
			if (!frame.vars.is_empty()) {
				jw.key("vars");
				jw.begin_object(); // vars {
				for (int j = 0; j < frame.vars.size(); j++) {
					jw.kv_variant(frame.vars[j].first, frame.vars[j].second);
				}
				jw.end_object(); // } vars
			}
			jw.end_object(); // } frame
		}
		jw.end_array(); // ] frames
		jw.end_object(); // } stacktrace
	}
	jw.end_object(); // } exception

	return jw.get_string();
}

void SentryEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &SentryEvent::get_id);
	ClassDB::bind_method(D_METHOD("set_message", "message"), &SentryEvent::set_message);
	ClassDB::bind_method(D_METHOD("get_message"), &SentryEvent::get_message);
	ClassDB::bind_method(D_METHOD("set_timestamp", "timestamp"), &SentryEvent::set_timestamp);
	ClassDB::bind_method(D_METHOD("get_timestamp"), &SentryEvent::get_timestamp);
	ClassDB::bind_method(D_METHOD("get_platform"), &SentryEvent::get_platform);
	ClassDB::bind_method(D_METHOD("set_level", "level"), &SentryEvent::set_level);
	ClassDB::bind_method(D_METHOD("get_level"), &SentryEvent::get_level);
	ClassDB::bind_method(D_METHOD("set_logger", "logger"), &SentryEvent::set_logger);
	ClassDB::bind_method(D_METHOD("get_logger"), &SentryEvent::get_logger);
	ClassDB::bind_method(D_METHOD("set_release", "release"), &SentryEvent::set_release);
	ClassDB::bind_method(D_METHOD("get_release"), &SentryEvent::get_release);
	ClassDB::bind_method(D_METHOD("set_dist", "dist"), &SentryEvent::set_dist);
	ClassDB::bind_method(D_METHOD("get_dist"), &SentryEvent::get_dist);
	ClassDB::bind_method(D_METHOD("set_environment", "environment"), &SentryEvent::set_environment);
	ClassDB::bind_method(D_METHOD("get_environment"), &SentryEvent::get_environment);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &SentryEvent::set_tag);
	ClassDB::bind_method(D_METHOD("remove_tag", "key"), &SentryEvent::remove_tag);
	ClassDB::bind_method(D_METHOD("get_tag", "key"), &SentryEvent::get_tag);
	ClassDB::bind_method(D_METHOD("is_crash"), &SentryEvent::is_crash);
	ClassDB::bind_method(D_METHOD("to_json"), &SentryEvent::to_json);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "id"), "", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "message"), "set_message", "get_message");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "timestamp"), "set_timestamp", "get_timestamp");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "platform"), "", "get_platform");
	ADD_PROPERTY(sentry::make_level_enum_property("level"), "set_level", "get_level");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "logger"), "set_logger", "get_logger");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "release"), "set_release", "get_release");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "dist"), "set_dist", "get_dist");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "environment"), "set_environment", "get_environment");

	ClassDB::bind_method(D_METHOD("get_exception_count"), &SentryEvent::get_exception_count);
	ClassDB::bind_method(D_METHOD("set_exception_value", "index", "value"), &SentryEvent::set_exception_value);
	ClassDB::bind_method(D_METHOD("get_exception_value", "index"), &SentryEvent::get_exception_value);
}

} // namespace sentry
