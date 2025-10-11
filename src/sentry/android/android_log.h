#pragma once

#include "sentry/sentry_log.h"

namespace sentry::android {

class AndroidLog : public SentryLog {
	GDCLASS(AndroidLog, SentryLog);

private:
	Object *android_plugin = nullptr;
	int32_t handle = 0;
	bool is_borrowed = false;

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

	void set_as_borrowed() { is_borrowed = true; }

	AndroidLog();
	AndroidLog(Object *p_android_plugin, int32_t p_handle);
	virtual ~AndroidLog() override;
};

} // namespace sentry::android
