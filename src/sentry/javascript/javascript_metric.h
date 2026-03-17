#pragma once

#include "sentry/javascript/javascript_interop.h"
#include "sentry/sentry_metric.h"

namespace sentry::javascript {

class JavaScriptMetric : public SentryMetric {
	GDCLASS(JavaScriptMetric, SentryMetric);

private:
	JSObjectPtr js_obj;

protected:
	static void _bind_methods() {}

public:
	_ALWAYS_INLINE_ JSObjectPtr get_js_object() const { return js_obj; }

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

	JavaScriptMetric(const JSObjectPtr &p_js_object);
	JavaScriptMetric();
	virtual ~JavaScriptMetric() override;
};

} //namespace sentry::javascript
