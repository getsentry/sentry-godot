#ifndef ANDROID_BREADCRUMB_H
#define ANDROID_BREADCRUMB_H

#include "sentry/sentry_breadcrumb.h"

namespace sentry::android {

class AndroidBreadcrumb : public SentryBreadcrumb {
	GDCLASS(AndroidBreadcrumb, SentryBreadcrumb);

private:
	Object *android_plugin = nullptr;
	int32_t handle = 0;

protected:
	static void _bind_methods() {}

public:
	int32_t get_handle() { return handle; }

	virtual void set_message(const String &p_message) override;
	virtual String get_message() const override;

	virtual void set_category(const String &p_category) override;
	virtual String get_category() const override;

	virtual void set_level(sentry::Level p_level) override;
	virtual sentry::Level get_level() const override;

	virtual void set_type(const String &p_type) override;
	virtual String get_type() const override;

	virtual void set_data(const Dictionary &p_data) override;

	virtual Ref<SentryTimestamp> get_timestamp() override;

	AndroidBreadcrumb() = default;
	AndroidBreadcrumb(Object *p_android_plugin, int32_t p_breadcrumb_handle);
	virtual ~AndroidBreadcrumb() override;
};

} //namespace sentry::android

#endif // ANDROID_BREADCRUMB_H
