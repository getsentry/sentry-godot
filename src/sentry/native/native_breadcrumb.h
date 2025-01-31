#ifndef NATIVE_BREADCRUMB_H
#define NATIVE_BREADCRUMB_H

#include "sentry_breadcrumb.h"

#include <sentry.h>

class NativeBreadcrumb : public SentryBreadcrumb {
	GDCLASS(NativeBreadcrumb, SentryBreadcrumb);

private:
	sentry_value_t native_crumb;

protected:
	static void _bind_methods() {}

public:
	_FORCE_INLINE_ sentry_value_t get_native_value() const { return native_crumb; }

	virtual void set_message(const String &p_message) override;
	virtual String get_message() const override;

	virtual void set_category(const String &p_category) override;
	virtual String get_category() const override;

	virtual void set_level(sentry::Level p_level) override;
	virtual sentry::Level get_level() const override;

	virtual void set_type(const String &p_type) override;
	virtual String get_type() const override;

	virtual void set_data(const Dictionary &p_data) override;
	virtual Dictionary get_data() const override;

	NativeBreadcrumb(const String &p_message, const String &p_category, sentry::Level p_level, const String &p_type, const Dictionary &p_data);
	NativeBreadcrumb(sentry_value_t p_native_crumb);
	NativeBreadcrumb();
	virtual ~NativeBreadcrumb() override;
};

#endif // NATIVE_BREADCRUMB_H
