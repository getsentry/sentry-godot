#include "cocoa_metric.h"

#include "cocoa_util.h"

namespace sentry::cocoa {

String CocoaMetric::get_name() const {
	ERR_FAIL_NULL_V(cocoa_metric, String());
	return string_from_objc(cocoa_metric.name);
}

void CocoaMetric::set_name(const String &p_name) {
	ERR_FAIL_NULL(cocoa_metric);
	cocoa_metric.name = string_to_objc(p_name);
}

CocoaMetric::MetricType CocoaMetric::get_type() const {
	ERR_FAIL_NULL_V(cocoa_metric, SentryMetric::METRIC_COUNTER);

	if (cocoa_metric.value.isCounter) {
		return SentryMetric::METRIC_COUNTER;
	} else if (cocoa_metric.value.isGauge) {
		return SentryMetric::METRIC_GAUGE;
	} else if (cocoa_metric.value.isDistribution) {
		return SentryMetric::METRIC_DISTRIBUTION;
	} else {
		WARN_PRINT("Sentry: Unexpected cocoa metric value type");
		return SentryMetric::METRIC_COUNTER;
	}
}

void CocoaMetric::set_type(MetricType p_type) {
	ERR_FAIL_NULL(cocoa_metric);

	double value = get_value();

	switch (p_type) {
		case SentryMetric::METRIC_COUNTER: {
			cocoa_metric.value = [SentryObjCMetricValue counter:(NSUInteger)MAX(value, 0.0)];
		} break;
		case SentryMetric::METRIC_GAUGE: {
			cocoa_metric.value = [SentryObjCMetricValue gauge:value];
		} break;
		case SentryMetric::METRIC_DISTRIBUTION: {
			cocoa_metric.value = [SentryObjCMetricValue distribution:value];
		} break;
		default: {
			WARN_PRINT("Sentry: Unexpected metric type");
		} break;
	}
}

double CocoaMetric::get_value() const {
	ERR_FAIL_NULL_V(cocoa_metric, 0.0);

	if (cocoa_metric.value.isCounter) {
		return (double)cocoa_metric.value.counterValue;
	} else if (cocoa_metric.value.isGauge) {
		return cocoa_metric.value.gaugeValue;
	} else {
		return cocoa_metric.value.distributionValue;
	}
}

void CocoaMetric::set_value(double p_value) {
	ERR_FAIL_NULL(cocoa_metric);

	if (cocoa_metric.value.isCounter) {
		cocoa_metric.value = [SentryObjCMetricValue counter:(NSUInteger)MAX(p_value, 0.0)];
	} else if (cocoa_metric.value.isGauge) {
		cocoa_metric.value = [SentryObjCMetricValue gauge:p_value];
	} else {
		cocoa_metric.value = [SentryObjCMetricValue distribution:p_value];
	}
}

String CocoaMetric::get_unit() const {
	ERR_FAIL_NULL_V(cocoa_metric, String());
	return cocoa_metric.unit ? string_from_objc(cocoa_metric.unit.rawValue) : String();
}

void CocoaMetric::set_unit(const String &p_unit) {
	ERR_FAIL_NULL(cocoa_metric);
	cocoa_metric.unit = p_unit.is_empty()
			? nil
			: [[SentryObjCUnit alloc] initWithRawValue:string_to_objc(p_unit)];
}

Variant CocoaMetric::get_attribute(const String &p_name) const {
	ERR_FAIL_NULL_V(cocoa_metric, Variant());

	SentryObjCAttributeContent *content = cocoa_metric.attributes[string_to_objc(p_name)];
	if (!content) {
		return Variant();
	}

	return variant_from_objc(content.value);
}

void CocoaMetric::set_attribute(const String &p_name, const Variant &p_value) {
	ERR_FAIL_NULL(cocoa_metric);

	NSMutableDictionary *mut_attributes = [cocoa_metric.attributes mutableCopy] ?: [NSMutableDictionary dictionary];
	[mut_attributes setObject:variant_to_attribute_content(p_value) forKey:string_to_objc(p_name)];
	cocoa_metric.attributes = mut_attributes;
}

void CocoaMetric::add_attributes(const Dictionary &p_attributes) {
	ERR_FAIL_NULL(cocoa_metric);

	NSMutableDictionary *mut_attributes = [cocoa_metric.attributes mutableCopy] ?: [NSMutableDictionary dictionary];
	const Array &keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		const Variant &key = keys[i];
		const Variant &value = p_attributes[key];
		[mut_attributes setObject:variant_to_attribute_content(value) forKey:string_to_objc(key.stringify())];
	}
	cocoa_metric.attributes = mut_attributes;
}

void CocoaMetric::remove_attribute(const String &p_name) {
	ERR_FAIL_NULL(cocoa_metric);

	NSMutableDictionary *mut_attributes = [cocoa_metric.attributes mutableCopy];
	[mut_attributes removeObjectForKey:string_to_objc(p_name)];
	cocoa_metric.attributes = mut_attributes;
}

CocoaMetric::CocoaMetric() :
		cocoa_metric(nil) {
	// NOTE: Required for ClassDB registration and compilation to succeed.
	ERR_PRINT("This constructor is not intended for runtime use.");
}

CocoaMetric::CocoaMetric(SentryObjCMetric *p_metric) :
		cocoa_metric(p_metric) {
}

CocoaMetric::~CocoaMetric() {
}

} // namespace sentry::cocoa
