#pragma once

#include "sentry/sentry_breadcrumb.h"

namespace sentry::javascript {

class JavaScriptBreadcrumb : public SentryBreadcrumb {
	GDCLASS(JavaScriptBreadcrumb, SentryBreadcrumb);

private:
	String message;
	String category;
	sentry::Level level;
	String type;
	Dictionary data;
	Ref<SentryTimestamp> timestamp;

protected:
	static void _bind_methods() {}

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

	virtual Ref<SentryTimestamp> get_timestamp() override {
		return timestamp;
	}

	String to_json() const;

	JavaScriptBreadcrumb();
	virtual ~JavaScriptBreadcrumb() override;
};

} //namespace sentry::javascript
