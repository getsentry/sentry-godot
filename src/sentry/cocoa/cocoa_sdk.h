#ifndef COCOA_SDK_H
#define COCOA_SDK_H

#include "sentry/internal_sdk.h"

namespace sentry {

// Internal SDK utilizing sentry-cocoa.
class CocoaSDK : public InternalSDK {
private:
	bool initialized = false;
	String last_uuid = "";
public:
	virtual void set_context(const String &p_key, const Dictionary &p_value) override;
	virtual void remove_context(const String &p_key) override;

	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void remove_tag(const String &p_key) override;

	virtual void set_user(const Ref<SentryUser> &p_user) override;
	virtual void remove_user() override;

	virtual void add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
			const String &p_type = "default", const Dictionary &p_data = Dictionary()) override;

	virtual String capture_message(const String &p_message, Level p_level = sentry::LEVEL_INFO) override;
	virtual String get_last_event_id() override;
	virtual String capture_error(const String &p_type, const String &p_value, Level p_level, const Vector<StackFrame> &p_frames) override;

	virtual Ref<SentryEvent> create_event() override;
	virtual String capture_event(const Ref<SentryEvent> &p_event) override;

	virtual void initialize() override;

	virtual ~CocoaSDK() override;
};

} //namespace sentry

#endif // NATIVE_SDK_H
