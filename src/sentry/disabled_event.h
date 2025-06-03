#ifndef DISABLED_EVENT_H
#define DISABLED_EVENT_H

#include "sentry_event.h"

// Event class that is used when the SDK is disabled.
class DisabledEvent : public SentryEvent {
	GDCLASS(DisabledEvent, SentryEvent);

private:
	String message;
	String timestamp;
	String logger;
	sentry::Level level = sentry::Level::LEVEL_INFO;
	String release;
	String dist;
	String environment;

protected:
	static void _bind_methods() {}

public:
	virtual String get_id() const override { return ""; }

	virtual void set_message(const String &p_message) override { message = p_message; }
	virtual String get_message() const override { return message; }

	virtual void set_timestamp(const String &p_timestamp) override { timestamp = p_timestamp; }
	virtual String get_timestamp() const override { return timestamp; }

	virtual String get_platform() const override { return ""; }

	virtual void set_level(sentry::Level p_level) override { level = p_level; }
	virtual sentry::Level get_level() const override { return level; }

	virtual void set_logger(const String &p_logger) override { logger = p_logger; }
	virtual String get_logger() const override { return logger; }

	virtual void set_release(const String &p_release) override { release = p_release; }
	virtual String get_release() const override { return release; }

	virtual void set_dist(const String &p_dist) override { dist = p_dist; }
	virtual String get_dist() const override { return dist; }

	virtual void set_environment(const String &p_environment) override { environment = p_environment; }
	virtual String get_environment() const override { return environment; }

	virtual void set_tag(const String &p_key, const String &p_value) override {}
	virtual void remove_tag(const String &p_key) override {}
	virtual String get_tag(const String &p_key) override { return ""; }

	virtual void add_exception(const Exception &p_exception) override {}

	virtual bool is_crash() const override { return false; }
};

#endif // DISABLED_EVENT_H
