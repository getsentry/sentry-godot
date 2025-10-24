#pragma once

#include "sentry/sentry_log.h"

#include "sentry.h"

namespace sentry::native {

class NativeLog : public SentryLog {
	GDCLASS(NativeLog, SentryLog);

private:
	sentry_value_t native_log;

protected:
	static void _bind_methods() {}

public:
	virtual LogLevel get_level() const override;
	virtual void set_level(LogLevel p_level) override;

	virtual String get_body() const override;
	virtual void set_body(const String &p_body) override;

	virtual Variant get_attribute(const String &p_name) const override;
	virtual void set_attribute(const String &p_name, const Variant &p_value) override;
	virtual void add_attributes(const Dictionary &p_attributes) override;
	virtual void remove_attribute(const String &p_name) override;

	NativeLog();
	NativeLog(sentry_value_t p_native_log);
	virtual ~NativeLog() override;
};

} //namespace sentry::native
