#ifndef DISABLED_BREADCRUMB_H
#define DISABLED_BREADCRUMB_H

#include "sentry/level.h"
#include "sentry_breadcrumb.h"

class DisabledBreadcrumb : public SentryBreadcrumb {
	GDCLASS(DisabledBreadcrumb, SentryBreadcrumb);

private:
	String message;
	String category;
	sentry::Level level = sentry::Level::LEVEL_INFO;
	String type;
	Dictionary data;

public:
	virtual void set_message(const String &p_message) override { message = p_message; }
	virtual String get_message() const override { return message; }

	virtual void set_category(const String &p_category) override { category = p_category; }
	virtual String get_category() const override { return category; }

	virtual void set_level(sentry::Level p_level) override { level = p_level; }
	virtual sentry::Level get_level() const override { return level; }

	virtual void set_type(const String &p_type) override { type = p_type; }
	virtual String get_type() const override { return type; }

	virtual void set_data(const Dictionary &p_data) override { data = p_data; }
	virtual Dictionary get_data() const override { return data; }
};

#endif // DISABLED_BREADCRUMB_H
