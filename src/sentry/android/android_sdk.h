#ifndef SENTRY_ANDROID_SDK_H
#define SENTRY_ANDROID_SDK_H

#include "sentry/internal_sdk.h"

using namespace godot;

namespace sentry::android {

class SentryAndroidBeforeSendHandler : public Object {
	GDCLASS(SentryAndroidBeforeSendHandler, Object);
	friend class AndroidSDK;

private:
	Object *android_plugin = nullptr;

	void _initialize(Object *p_android_plugin);

	void _before_send(int32_t p_event_handle);

protected:
	static void _bind_methods();
};

class SentryAndroidBeforeSendLogHandler : public Object {
	GDCLASS(SentryAndroidBeforeSendLogHandler, Object);
	friend class AndroidSDK;

private:
	Object *android_plugin = nullptr;

	void _initialize(Object *p_android_plugin);

	void _before_send_log(int32_t p_log_handle);

protected:
	static void _bind_methods();
};

// Internal SDK utilizing Sentry Android (sentry-java repo).
class AndroidSDK : public InternalSDK {
private:
	Object *android_plugin = nullptr;
	SentryAndroidBeforeSendHandler *before_send_handler = nullptr;
	SentryAndroidBeforeSendLogHandler *before_send_log_handler = nullptr;

public:
	virtual void set_context(const String &p_key, const Dictionary &p_value) override;
	virtual void remove_context(const String &p_key) override;

	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void remove_tag(const String &p_key) override;

	virtual void set_user(const Ref<SentryUser> &p_user) override;
	virtual void remove_user() override;

	virtual Ref<SentryBreadcrumb> create_breadcrumb() override;
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) override;

	virtual void log(LogLevel p_level, const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary()) override;

	virtual String capture_message(const String &p_message, Level p_level = sentry::LEVEL_INFO) override;
	virtual String get_last_event_id() override;

	virtual Ref<SentryEvent> create_event() override;
	virtual String capture_event(const Ref<SentryEvent> &p_event) override;

	virtual void add_attachment(const Ref<SentryAttachment> &p_attachment) override;

	virtual void init(const PackedStringArray &p_global_attachments, const Callable &p_configuration_callback) override;
	virtual void close() override;
	virtual bool is_enabled() const override;

	bool has_android_plugin() const { return android_plugin != nullptr; }

	AndroidSDK();
	virtual ~AndroidSDK() override;
};

} //namespace sentry::android

#endif // SENTRY_ANDROID_SDK_H
