#ifndef SENTRY_ANDROID_EVENT_H
#define SENTRY_ANDROID_EVENT_H

#include "sentry_event.h"

using namespace godot;

// Event class that is used with the AndroidSDK.
class AndroidEvent : public SentryEvent {
	GDCLASS(AndroidEvent, SentryEvent);

private:
	Object *android_plugin = nullptr;
	int32_t event_handle;
	bool is_borrowed = false;

protected:
	static void _bind_methods() {}

public:
	int32_t get_handle() { return event_handle; }

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

	virtual void merge_context(const String &p_key, const Dictionary &p_value) override;

	virtual void add_exception(const Exception &p_exception) override;

	virtual bool is_crash() const override;

	void set_as_borrowed() { is_borrowed = true; }

	AndroidEvent() {}
	AndroidEvent(Object *android_plugin, int32_t p_event_handle);
	virtual ~AndroidEvent() override;
};

#endif // SENTRY_ANDROID_EVENT_H
