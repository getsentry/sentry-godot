#ifndef NATIVE_EVENT_H
#define NATIVE_EVENT_H

#include "sentry_event.h"

#include <sentry.h>

namespace sentry {

// Event class that is used with the NativeSDK.
class NativeEvent : public SentryEvent {
	GDCLASS(NativeEvent, SentryEvent);

private:
	sentry_value_t native_event;

protected:
	static void _bind_methods() {}

public:
	sentry_value_t get_native_value() const { return native_event; }

	virtual String get_id() const override;

	virtual void set_message(const String &p_message) override;
	virtual String get_message() const override;

	virtual void set_timestamp(const String &p_timestamp) override;
	virtual String get_timestamp() const override;

	virtual String get_platform() const override;

	virtual void set_level(sentry::Level p_level) override;
	virtual sentry::Level get_level() const override;

	virtual void set_logger(const String &p_logger) override;
	virtual String get_logger() const override;

	virtual void set_release(const String &p_release) override;
	virtual String get_release() const override;

	virtual void set_dist(const String &p_dist) override;
	virtual String get_dist() const override;

	virtual void set_environment(const String &p_environment) override;
	virtual String get_environment() const override;

	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void remove_tag(const String &p_key) override;
	virtual String get_tag(const String &p_key) override;

	NativeEvent(sentry_value_t p_event);
	NativeEvent();
	virtual ~NativeEvent() override;
};

} // namespace sentry

#endif // NATIVE_EVENT_H
