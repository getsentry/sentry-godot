#include "native_metric.h"

#include "native_util.h"

namespace sentry::native {

String NativeMetric::get_name() const {
	sentry_value_t name = sentry_value_get_by_key(native_metric, "name");
	return String::utf8(sentry_value_as_string(name));
}

void NativeMetric::set_name(const String &p_name) {
	sentry_value_set_by_key(native_metric, "name",
			sentry_value_new_string(p_name.utf8()));
}

NativeMetric::MetricType NativeMetric::get_type() const {
	sentry_value_t type = sentry_value_get_by_key(native_metric, "type");
	const char *type_cstr = sentry_value_as_string(type);

	if (strcmp(type_cstr, "counter") == 0) {
		return SentryMetric::METRIC_COUNTER;
	} else if (strcmp(type_cstr, "gauge") == 0) {
		return SentryMetric::METRIC_GAUGE;
	} else if (strcmp(type_cstr, "distribution") == 0) {
		return SentryMetric::METRIC_DISTRIBUTION;
	} else {
		WARN_PRINT("Sentry: Unexpected native metric type string \"" + String(type_cstr) + "\"");
		return SentryMetric::METRIC_COUNTER;
	}
}

void NativeMetric::set_type(MetricType p_type) {
	switch (p_type) {
		case SentryMetric::METRIC_COUNTER: {
			sentry_value_set_by_key(native_metric, "type",
					sentry_value_new_string("counter"));
		} break;
		case SentryMetric::METRIC_GAUGE: {
			sentry_value_set_by_key(native_metric, "type",
					sentry_value_new_string("gauge"));
		} break;
		case SentryMetric::METRIC_DISTRIBUTION: {
			sentry_value_set_by_key(native_metric, "type",
					sentry_value_new_string("distribution"));
		} break;
		default: {
			WARN_PRINT("Sentry: Unexpected metric type");
		}
	}
}

double NativeMetric::get_value() const {
	sentry_value_t value = sentry_value_get_by_key(native_metric, "value");
	sentry_value_type_t value_type = sentry_value_get_type(value);
	if (value_type == SENTRY_VALUE_TYPE_DOUBLE) {
		return sentry_value_as_double(value);
	} else {
		return sentry_value_as_int64(value);
	}
}

void NativeMetric::set_value(double p_value) {
	if (get_type() == METRIC_COUNTER) {
		// Counter values are stored as integers.
		sentry_value_set_by_key(native_metric, "value",
				sentry_value_new_int64(p_value));
	} else {
		sentry_value_set_by_key(native_metric, "value",
				sentry_value_new_double(p_value));
	}
}

String NativeMetric::get_unit() const {
	return String::utf8(sentry_value_as_string(
			sentry_value_get_by_key(native_metric, "unit")));
}

void NativeMetric::set_unit(const String &p_unit) {
	sentry_value_set_by_key(native_metric, "unit",
			sentry_value_new_string(p_unit.utf8()));
}

Variant NativeMetric::get_attribute(const String &p_name) const {
	return sentry_value_get_attribute(native_metric, p_name);
}

void NativeMetric::set_attribute(const String &p_name, const Variant &p_value) {
	sentry_value_set_attribute(native_metric, p_name, p_value);
}

void NativeMetric::add_attributes(const Dictionary &p_attributes) {
	sentry_value_add_attributes(native_metric, p_attributes);
}

void NativeMetric::remove_attribute(const String &p_name) {
	sentry_value_t attributes = sentry_value_get_by_key(native_metric, "attributes");
	sentry_value_remove_by_key(attributes, p_name.utf8());
}

NativeMetric::NativeMetric() {
	native_metric = sentry_value_new_object();
	// NOTE: Required for ClassDB registration and compilation to succeed.
	ERR_PRINT("This constructor is not intended for runtime use.");
}

NativeMetric::NativeMetric(sentry_value_t p_native_metric) :
		native_metric(p_native_metric) {
	sentry_value_incref(p_native_metric);
}

NativeMetric::~NativeMetric() {
	sentry_value_decref(native_metric);
}

} // namespace sentry::native
