#pragma once

#include "sentry/sentry_event_impl.h"
#include "sentry/util/object_handle.h"

using namespace godot;

namespace sentry::android {

// Event class that is used with the AndroidSDK.
class AndroidEvent : public SentryEventImpl {
	SENTRY_CASTABLE(AndroidEvent, SentryEventImpl);

private:
	util::ObjectHandle<Object> _android_plugin;
	int32_t event_handle;
	bool is_borrowed = false;

public:
	int32_t get_handle() { return event_handle; }

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

	virtual void set_user(const Ref<SentryUser> &p_user) override;

	virtual void set_fingerprint(const PackedStringArray &p_fingerprint) override;

	virtual void set_context(const String &p_key, const Dictionary &p_value) override;
	virtual void merge_context(const String &p_key, const Dictionary &p_value) override;

	virtual void add_exception(const Exception &p_exception) override;

	virtual int get_exception_count() const override;
	virtual void set_exception_value(int p_index, const String &p_value) override;
	virtual String get_exception_value(int p_index) const override;

	virtual bool is_crash() const override;

	virtual String to_json() const override;

	static AndroidEvent *create_borrowed(util::ObjectHandle<Object> p_android_plugin, int32_t p_event_handle);

	AndroidEvent(util::ObjectHandle<Object> p_android_plugin, int32_t p_event_handle);
	virtual ~AndroidEvent() override;
};

} //namespace sentry::android
