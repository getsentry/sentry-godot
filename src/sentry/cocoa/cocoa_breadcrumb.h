#ifndef COCOA_BREADCRUMB_H
#define COCOA_BREADCRUMB_H

#include "sentry/cocoa/cocoa_includes.h"
#include "sentry/sentry_breadcrumb.h"

namespace sentry::cocoa {

class CocoaBreadcrumb : public SentryBreadcrumb {
	GDCLASS(CocoaBreadcrumb, SentryBreadcrumb);

private:
	objc::SentryBreadcrumb *cocoa_breadcrumb = nullptr;

protected:
	static void _bind_methods() {}

public:
	_FORCE_INLINE_ objc::SentryBreadcrumb *get_cocoa_breadcrumb() const { return cocoa_breadcrumb; }

	virtual void set_message(const String &p_message) override;
	virtual String get_message() const override;

	virtual void set_category(const String &p_category) override;
	virtual String get_category() const override;

	virtual void set_level(sentry::Level p_level) override;
	virtual sentry::Level get_level() const override;

	virtual void set_type(const String &p_type) override;
	virtual String get_type() const override;

	virtual void set_data(const Dictionary &p_data) override;

	CocoaBreadcrumb();
	CocoaBreadcrumb(objc::SentryBreadcrumb *p_cocoa_breadcrumb);
};

} //namespace sentry::cocoa

#endif // COCOA_BREADCRUMB_H
