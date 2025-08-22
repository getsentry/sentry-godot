#ifndef COCOA_EVENT_H
#define COCOA_EVENT_H

#include "sentry/cocoa/cocoa_includes.h"
#include "sentry/sentry_event.h"

namespace sentry::cocoa {

class CocoaEvent : public sentry::SentryEvent {
	GDCLASS(CocoaEvent, sentry::SentryEvent);

private:
	objc::SentryEvent *cocoa_event = nullptr;

protected:
	static void _bind_methods() {}

public:
	_FORCE_INLINE_ objc::SentryEvent *get_cocoa_event() const { return cocoa_event; }

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

	virtual bool is_crash() const override;

	virtual String to_json() const override;

	CocoaEvent(objc::SentryEvent *p_cocoa_event);
	CocoaEvent();
	virtual ~CocoaEvent() override;
};

} // namespace sentry::cocoa

#endif // COCOA_EVENT_H
