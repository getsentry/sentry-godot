#ifndef COCOA_EVENT_H
#define COCOA_EVENT_H

#include "sentry_event.h"

namespace sentry {

// Event class that is used with the Cocoa SDK.
class CocoaEvent : public sentry::SentryEvent {
	GDCLASS(CocoaEvent, SentryEvent);

private:
	void *cocoa_event;

protected:
	static void _bind_methods() {}

public:
    void* get_native_value() const { return cocoa_event; };

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

	CocoaEvent(void* p_event);
	CocoaEvent();
	virtual ~CocoaEvent() override;
};


}

#endif // COCOA_EVENT_H
