#include "sentry_metrics.h"

#include "sentry/sentry_sdk.h"

namespace {

inline bool _is_valid_value(const Variant &p_value) {
	return p_value.get_type() == Variant::INT || p_value.get_type() == Variant::FLOAT;
}

} // unnamed namespace

namespace sentry {

void SentryMetrics::count(const String &p_name, const Variant &p_value, const Dictionary &p_attributes) {
	ERR_FAIL_COND_MSG(p_name.is_empty(), "SentryMetrics.count(): metric name must not be empty.");
	ERR_FAIL_COND_MSG(!_is_valid_value(p_value), "SentryMetrics.count(): expected an int or float value, got " + Variant::get_type_name(p_value.get_type()) + ".");
}

void SentryMetrics::gauge(const String &p_name, const Variant &p_value, const String &p_unit, const Dictionary &p_attributes) {
	ERR_FAIL_COND_MSG(p_name.is_empty(), "SentryMetrics.gauge(): metric name must not be empty.");
	ERR_FAIL_COND_MSG(!_is_valid_value(p_value), "SentryMetrics.gauge(): expected an int or float value, got " + Variant::get_type_name(p_value.get_type()) + ".");
}

void SentryMetrics::distribution(const String &p_name, const Variant &p_value, const String &p_unit, const Dictionary &p_attributes) {
	ERR_FAIL_COND_MSG(p_name.is_empty(), "SentryMetrics.distribution(): metric name must not be empty.");
	ERR_FAIL_COND_MSG(!_is_valid_value(p_value), "SentryMetrics.distribution(): expected an int or float value, got " + Variant::get_type_name(p_value.get_type()) + ".");
}

void SentryMetrics::_bind_methods() {
	ClassDB::bind_method(D_METHOD("count", "name", "value", "attributes"), &SentryMetrics::count);
	ClassDB::bind_method(D_METHOD("gauge", "name", "value", "unit", "attributes"), &SentryMetrics::gauge);
	ClassDB::bind_method(D_METHOD("distribution", "name", "value", "unit", "attributes"), &SentryMetrics::distribution);
}

} // namespace sentry
