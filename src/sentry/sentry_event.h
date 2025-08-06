#ifndef SENTRY_EVENT_H
#define SENTRY_EVENT_H

#include "sentry/level.h"
#include "sentry/sentry_timestamp.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/pair.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry {

// Base class for event objects in the public API.
class SentryEvent : public RefCounted {
	GDCLASS(SentryEvent, RefCounted);

public:
	// Represents a frame of a stack trace.
	struct StackFrame {
		String filename;
		String function;
		int lineno = -1;
		bool in_app = true;
		String platform;
		String context_line;
		Vector<Pair<String, Variant>> vars;
		PackedStringArray pre_context;
		PackedStringArray post_context;
	};

	struct Exception {
		String type;
		String value;
		Vector<StackFrame> frames;
	};

protected:
	static void _bind_methods();

public:
	virtual String get_id() const = 0;

	virtual void set_message(const String &p_message) = 0;
	virtual String get_message() const = 0;

	virtual void set_timestamp(const Ref<SentryTimestamp> &p_timestamp) = 0;
	virtual Ref<SentryTimestamp> get_timestamp() const = 0;

	virtual String get_platform() const = 0;

	virtual void set_level(sentry::Level p_level) = 0;
	virtual sentry::Level get_level() const = 0;

	virtual void set_logger(const String &p_logger) = 0;
	virtual String get_logger() const = 0;

	virtual void set_release(const String &p_release) = 0;
	virtual String get_release() const = 0;

	virtual void set_dist(const String &p_dist) = 0;
	virtual String get_dist() const = 0;

	virtual void set_environment(const String &p_environment) = 0;
	virtual String get_environment() const = 0;

	virtual void set_tag(const String &p_key, const String &p_value) = 0;
	virtual void remove_tag(const String &p_key) = 0;
	virtual String get_tag(const String &p_key) = 0;

	virtual void merge_context(const String &p_key, const Dictionary &p_value) = 0;

	virtual void add_exception(const Exception &p_exception) = 0;

	virtual bool is_crash() const = 0;

	virtual ~SentryEvent() = default;
};

} // namespace sentry

#endif // SENTRY_EVENT_H
