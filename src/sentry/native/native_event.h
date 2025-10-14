#ifndef NATIVE_EVENT_H
#define NATIVE_EVENT_H

#include "sentry/sentry_event.h"

#include <sentry.h>

namespace sentry::native {

// Event class that is used with the NativeSDK.
class NativeEvent : public SentryEvent {
	GDCLASS(NativeEvent, SentryEvent);

private:
	sentry_value_t native_event;
	bool _is_crash = false;

protected:
	static void _bind_methods() {}

public:
	sentry_value_t get_native_value() const { return native_event; }

	virtual String get_id() const override;

	virtual void set_message(const String &p_message) override;
	virtual String get_message() const override;

	virtual void set_timestamp(const Ref<SentryTimestamp> &p_timestamp) override;
	virtual Ref<SentryTimestamp> get_timestamp() const override;

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

	virtual int get_exception_count() const override;
	virtual void set_exception_value(int p_index, const String &p_value) override;
	virtual String get_exception_value(int p_index) const override;

	virtual bool is_crash() const override;

	virtual String to_json() const override;

	NativeEvent(sentry_value_t p_event, bool p_is_crash);
	NativeEvent();
	virtual ~NativeEvent() override;
};

} //namespace sentry::native

#endif // NATIVE_EVENT_H
