#pragma once

#include "sentry/sentry_breadcrumb.h"

namespace sentry::javascript {

class JavaScriptSDK;

class JavaScriptBreadcrumb : public SentryBreadcrumb {
	GDCLASS(JavaScriptBreadcrumb, SentryBreadcrumb);

	friend class JavaScriptSDK;

private:
	Ref<RefCounted> js_obj;

	_FORCE_INLINE_ Ref<RefCounted> get_js_object() const { return js_obj; }

protected:
	static void _bind_methods() {}

public:
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

	JavaScriptBreadcrumb();
	virtual ~JavaScriptBreadcrumb() override;
};

} //namespace sentry::javascript
