#pragma once

#include "sentry/javascript/javascript_interop.h"
#include "sentry/sentry_scope_impl.h"

namespace sentry::javascript {

class JavaScriptScope : public SentryScopeImpl {
private:
	JSObjectPtr js_obj;

public:
	_FORCE_INLINE_ JSObjectPtr get_js_object() const { return js_obj; }

	virtual void set_context(const String &p_key, const Dictionary &p_value) override;
	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void set_user(const Ref<SentryUser> &p_user) override;
	virtual void set_level(sentry::Level p_level) override;
	virtual void set_fingerprint(const PackedStringArray &p_fingerprint) override;
	virtual void set_attribute(const String &p_name, const Variant &p_value) override;
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) override;
	virtual void clear() override;
	virtual SentryScopeImpl *clone() const override;

	explicit JavaScriptScope(const JSObjectPtr &p_js_scope_object);
	JavaScriptScope();
};

} //namespace sentry::javascript
