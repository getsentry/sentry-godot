#pragma once

#include "cocoa_includes.h"
#include "sentry/sentry_scope_impl.h"

namespace sentry::cocoa {

class CocoaScope : public SentryScopeImpl {
	SENTRY_CASTABLE(CocoaScope, SentryScopeImpl);

private:
	SentryObjCScope *_scope;

public:
	_FORCE_INLINE_ SentryObjCScope *get_cocoa_scope() const { return _scope; }

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

	CocoaScope(SentryObjCScope *p_scope);
	virtual ~CocoaScope() override;
};

} //namespace sentry::cocoa
