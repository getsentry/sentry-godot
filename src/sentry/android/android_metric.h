#pragma once

#include "sentry/sentry_metric.h"

namespace sentry::android {

class AndroidMetric : public SentryMetric {
	GDCLASS(AndroidMetric, SentryMetric);

private:
	Object *android_plugin = nullptr;
	int32_t handle = 0;
	bool is_borrowed = false;

protected:
	static void _bind_methods() {}

public:
	void set_as_borrowed() { is_borrowed = true; }

	virtual String get_name() const override;
	virtual void set_name(const String &p_name) override;

	virtual MetricType get_type() const override;
	virtual void set_type(MetricType p_type) override;

	virtual double get_value() const override;
	virtual void set_value(double p_value) override;

	virtual String get_unit() const override;
	virtual void set_unit(const String &p_unit) override;

	virtual Variant get_attribute(const String &p_name) const override;
	virtual void set_attribute(const String &p_name, const Variant &p_value) override;
	virtual void add_attributes(const Dictionary &p_attributes) override;
	virtual void remove_attribute(const String &p_name) override;

	AndroidMetric();
	AndroidMetric(Object *p_android_plugin, int32_t p_handle);
	virtual ~AndroidMetric() override;
};

} // namespace sentry::android
