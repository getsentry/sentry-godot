#pragma once

#include "sentry/cocoa/cocoa_includes.h"
#include "sentry/sentry_breadcrumb_impl.h"

namespace sentry::cocoa {

class CocoaBreadcrumb : public SentryBreadcrumbImpl {
	SENTRY_CASTABLE(CocoaBreadcrumb, SentryBreadcrumbImpl);

private:
	SentryObjCBreadcrumb *cocoa_breadcrumb = nullptr;

public:
	_FORCE_INLINE_ SentryObjCBreadcrumb *get_cocoa_breadcrumb() const { return cocoa_breadcrumb; }

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

	CocoaBreadcrumb();
	CocoaBreadcrumb(SentryObjCBreadcrumb *p_cocoa_breadcrumb);
};

} //namespace sentry::cocoa
