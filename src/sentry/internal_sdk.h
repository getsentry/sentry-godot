#ifndef INTERNAL_SDK_H
#define INTERNAL_SDK_H

#include "sentry/level.h"
#include "sentry_event.h"
#include "sentry_user.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry {

// Interface for SDKs used internally.
class InternalSDK {
public:
	// Represents a frame of a stack trace.
	struct StackFrame {
		String filename;
		String function;
		int lineno = -1;
		String context_line;
		PackedStringArray pre_context;
		PackedStringArray post_context;
	};

public:
	virtual void set_context(const String &p_key, const Dictionary &p_value) = 0;
	virtual void remove_context(const String &p_key) = 0;

	virtual void set_tag(const String &p_key, const String &p_value) = 0;
	virtual void remove_tag(const String &p_key) = 0;

	virtual void set_user(const Ref<SentryUser> &p_user) = 0;
	virtual void remove_user() = 0;

	virtual void add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
			const String &p_type = "default", const Dictionary &p_data = Dictionary()) = 0;
	// TODO: Consider adding the following function.
	// virtual void clear_breadcrumbs() = 0;

	virtual String capture_message(const String &p_message, Level p_level, const String &p_logger = "") = 0;
	virtual String get_last_event_id() = 0;
	virtual String capture_error(const String &p_type, const String &p_value, Level p_level, const Vector<StackFrame> &p_frames) = 0;

	virtual Ref<SentryEvent> create_event() = 0;
	virtual Ref<SentryEvent> create_message_event(const String &p_message, sentry::Level p_level, const String &p_logger = "") = 0;
	virtual String capture_event(const Ref<SentryEvent> &p_event) = 0;

	virtual void initialize() = 0;
	virtual ~InternalSDK() = default;
};

} //namespace sentry

#endif // INTERNAL_SDK_H
