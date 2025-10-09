#pragma once

#include "sentry/cocoa/cocoa_includes.h"
#include "sentry/sentry_log.h"

namespace sentry::cocoa {

class CocoaLog : public sentry::SentryLog {
	GDCLASS(CocoaLog, SentryLog)
private:
	objc::SentryLog *cocoa_log;

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
	virtual void remove_attribute(const String &p_name) const override;

	CocoaLog();
	CocoaLog(objc::SentryLog *p_log);
	virtual ~CocoaLog() override;
};

} // namespace sentry::cocoa
