#ifndef SENTRY_EVENT_H
#define SENTRY_EVENT_H

#include "sentry/level.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Base class for event objects in the public API.
class SentryEvent : public RefCounted {
	GDCLASS(SentryEvent, RefCounted);

protected:
	static void _bind_methods();

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

	virtual ~SentryEvent() = default;
};

#endif // SENTRY_EVENT_H
