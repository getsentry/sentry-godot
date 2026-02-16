#pragma once

#include "sentry/sentry_log.h"

namespace sentry::javascript {

class JavaScriptLog : public SentryLog {
	GDCLASS(JavaScriptLog, SentryLog);

private:
	Ref<RefCounted> js_obj;

protected:
	static void _bind_methods() {}

public:
	_FORCE_INLINE_ Ref<RefCounted> get_js_object() const { return js_obj; }

	virtual LogLevel get_level() const override;
	virtual void set_level(LogLevel p_level) override;

	virtual String get_body() const override;
	virtual void set_body(const String &p_body) override;

	virtual Variant get_attribute(const String &p_name) const override;
	virtual void set_attribute(const String &p_name, const Variant &p_value) override;
	virtual void add_attributes(const Dictionary &p_attributes) override;
	virtual void remove_attribute(const String &p_name) override;

	JavaScriptLog(const Ref<RefCounted> &p_js_log_object);
	JavaScriptLog();
	virtual ~JavaScriptLog() override;
};

} //namespace sentry::javascript
