#pragma once

#include "sentry/sentry_scope_impl.h"

#include "sentry.h"

namespace sentry::native {

// Thin wrapper around sentry-native scope used for SentryScope implementation on Windows and Linux.
class NativeScope : public SentryScopeImpl {
	SENTRY_CASTABLE(NativeScope, SentryScopeImpl);

private:
	sentry_scope_t *_scope;

public:
	sentry_scope_t *get_native_scope() const { return _scope; }

	virtual void set_context(const String &p_key, const Dictionary &p_value) override;
	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void set_user(const Ref<SentryUser> &p_user) override;
	virtual void set_level(sentry::Level p_level) override;
	virtual void set_fingerprint(const PackedStringArray &p_fingerprint) override;
	virtual void set_attribute(const String &p_name, const Variant &p_value) override;
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) override;
	virtual void add_attachment(const Ref<SentryAttachment> &p_attachment) override;
	virtual void clear() override;
	virtual SentryScopeImpl *clone() const override;
	virtual void set_span(SentrySpanImpl *p_span) override;

	NativeScope();
	NativeScope(sentry_scope_t *p_scope);
	virtual ~NativeScope() override;
};

} //namespace sentry::native
