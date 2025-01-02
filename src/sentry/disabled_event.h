#ifndef DISABLED_EVENT_H
#define DISABLED_EVENT_H

#include "sentry_event.h"

// Event class that is used when the SDK is disabled.
class DisabledEvent : public SentryEvent {
	GDCLASS(DisabledEvent, SentryEvent);

private:
	String message;
	String timestamp;
	sentry::Level level = sentry::Level::LEVEL_INFO;

protected:
	static void _bind_methods() {}

public:
	virtual String get_id() const override { return ""; }

	virtual void set_message(const String &p_message) override { message = p_message; }
	virtual String get_message() const override { return message; }

	virtual void set_timestamp(const String &p_timestamp) override { timestamp = p_timestamp; }
	virtual String get_timestamp() const override { return timestamp; }

	virtual void set_level(sentry::Level p_level) override { level = p_level; }
	virtual sentry::Level get_level() const override { return level; }
};

#endif // DISABLED_EVENT_H
