#include "sentry/sentry_metric.h"

#include "sentry.h"

namespace sentry::native {

class NativeMetric : public SentryMetric {
	GDCLASS(NativeMetric, SentryMetric);

private:
	sentry_value_t native_metric;

protected:
	static void _bind_methods() {}

public:
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

	NativeMetric();
	NativeMetric(sentry_value_t p_native_metric);
	virtual ~NativeMetric() override;
};

} // namespace sentry::native
