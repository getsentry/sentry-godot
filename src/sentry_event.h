#ifndef SENTRY_EVENT_H
#define SENTRY_EVENT_H

#include "sentry/level.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Base class for event objects in the public API.
class SentryEvent : public RefCounted {
	GDCLASS(SentryEvent, RefCounted);

public:
	enum EventType {
		EVENT_CUSTOM,
		EVENT_CRASH,
		EVENT_ERROR
	};

private:
	EventType event_type = EVENT_CUSTOM;

protected:
	static void _bind_methods();

	_FORCE_INLINE_ void _set_event_type(EventType p_type) { event_type = p_type; }

public:
	virtual String get_id() const = 0;

	virtual void set_message(const String &p_message) = 0;
	virtual String get_message() const = 0;

	virtual void set_timestamp(const String &p_timestamp) = 0;
	virtual String get_timestamp() const = 0;

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

	_FORCE_INLINE_ EventType get_event_type() const { return event_type; }

	virtual ~SentryEvent() = default;
};

VARIANT_ENUM_CAST(SentryEvent::EventType);

#endif // SENTRY_EVENT_H
