#ifndef NATIVE_SDK_H
#define NATIVE_SDK_H

#include "sentry/internal_sdk.h"

#include <sentry.h>
#include <godot_cpp/classes/mutex.hpp>

namespace sentry::native {

// Internal SDK utilizing sentry-native.
class NativeSDK : public InternalSDK {
private:
	sentry_uuid_t last_uuid;
	Ref<Mutex> last_uuid_mutex;
	bool initialized = false;

public:
	virtual void set_context(const String &p_key, const Dictionary &p_value) override;
	virtual void remove_context(const String &p_key) override;

	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void remove_tag(const String &p_key) override;

	virtual void set_user(const Ref<SentryUser> &p_user) override;
	virtual void remove_user() override;

	virtual Ref<SentryBreadcrumb> create_breadcrumb() override;
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) override;

	virtual String capture_message(const String &p_message, Level p_level = sentry::LEVEL_INFO) override;
	virtual String get_last_event_id() override;

	virtual Ref<SentryEvent> create_event() override;
	virtual String capture_event(const Ref<SentryEvent> &p_event) override;

	virtual void add_attachment(const Ref<SentryAttachment> &p_attachment) override;

	virtual void init(const PackedStringArray &p_global_attachments) override;
	virtual void close() override;
	virtual bool is_enabled() const override;

	NativeSDK();
	virtual ~NativeSDK() override;
};

} //namespace sentry::native

#endif // NATIVE_SDK_H
