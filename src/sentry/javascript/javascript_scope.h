#pragma once

#include "sentry/javascript/javascript_interop.h"
#include "sentry/sentry_scope_impl.h"

namespace sentry::javascript {

// Backed by a JS Scope object, which is guaranteed to be valid: creation failures
// yield a DisabledScope instead.
// See JavaScriptSDK::create_scope().
class JavaScriptScope : public SentryScopeImpl {
	SENTRY_CASTABLE(JavaScriptScope, SentryScopeImpl);

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
	virtual void add_attachment(const Ref<SentryAttachment> &p_attachment) override;
	virtual void set_span(SentrySpanImpl *p_span) override;
	virtual void clear() override;
	virtual SentryScopeImpl *clone() const override;

	explicit JavaScriptScope(const JSObjectPtr &p_js_scope_object);
	JavaScriptScope() = delete;
};

} //namespace sentry::javascript
