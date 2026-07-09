#pragma once

#include "sentry/sentry_scope_impl.h"

namespace sentry::android {

class AndroidScope : public SentryScopeImpl {
private:
	Object *android_plugin = nullptr;
	int32_t handle = 0;

public:
	int32_t get_handle() const { return handle; }

	virtual void set_context(const String &p_key, const Dictionary &p_value) override;
	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void set_user(const Ref<SentryUser> &p_user) override;
	virtual void set_level(sentry::Level p_level) override;
	virtual void set_fingerprint(const PackedStringArray &p_fingerprint) override;
	virtual void set_attribute(const String &p_name, const Variant &p_value) override;
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) override;
	virtual void clear() override;
	virtual SentryScopeImpl *clone() const override;

	AndroidScope() = default;
	AndroidScope(Object *p_android_plugin, int32_t p_handle);
	virtual ~AndroidScope() override;
};

} //namespace sentry::android
