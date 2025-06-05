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
	virtual void set_context(const String &p_key, const Dictionary &p_value) = 0;
	virtual void remove_context(const String &p_key) = 0;

	virtual void set_tag(const String &p_key, const String &p_value) = 0;
	virtual void remove_tag(const String &p_key) = 0;

	virtual void set_user(const Ref<SentryUser> &p_user) = 0;
	virtual void remove_user() = 0;

	virtual void add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
			const String &p_type = "default", const Dictionary &p_data = Dictionary()) = 0;

	virtual String capture_message(const String &p_message, Level p_level) = 0;
	virtual String get_last_event_id() = 0;

	virtual Ref<SentryEvent> create_event() = 0;
	virtual String capture_event(const Ref<SentryEvent> &p_event) = 0;

	virtual void initialize(const PackedStringArray &p_global_attachments) = 0;

	virtual ~InternalSDK() = default;
};

} //namespace sentry

#endif // INTERNAL_SDK_H
