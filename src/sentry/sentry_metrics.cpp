#include "sentry_metrics.h"

#include "sentry/sentry_sdk.h"

namespace {

inline bool _is_valid_value(const Variant &p_value) {
	return p_value.get_type() == Variant::INT || p_value.get_type() == Variant::FLOAT;
}

} // unnamed namespace

namespace sentry {

void SentryMetrics::counter(const String &p_name, Variant p_value, const Dictionary &p_attributes) {
	if (!_is_valid_value(p_value)) {
		ERR_PRINT("SentryMetrics.counter(): expected an int or float value, got " + Variant::get_type_name(p_value.get_type()) + ".");
		return;
	}
}

void SentryMetrics::gauge(const String &p_name, Variant p_value, const String &p_unit, const Dictionary &p_attributes) {
	if (!_is_valid_value(p_value)) {
		ERR_PRINT("SentryMetrics.gauge(): expected an int or float value, got " + Variant::get_type_name(p_value.get_type()) + ".");
		return;
	}
}

void SentryMetrics::distribution(const String &p_name, Variant p_value, const String &p_unit, const Dictionary &p_attributes) {
	if (!_is_valid_value(p_value)) {
		ERR_PRINT("SentryMetrics.distribution(): expected an int or float value, got " + Variant::get_type_name(p_value.get_type()) + ".");
		return;
	}
}

void SentryMetrics::_bind_methods() {
	ClassDB::bind_method(D_METHOD("counter", "name", "value", "attributes"), &SentryMetrics::counter);
	ClassDB::bind_method(D_METHOD("gauge", "name", "value", "unit", "attributes"), &SentryMetrics::gauge);
	ClassDB::bind_method(D_METHOD("distribution", "name", "value", "unit", "attributes"), &SentryMetrics::distribution);
}

} // namespace sentry
