#include "sentry_metrics.h"

#include "sentry/sentry_sdk.h"

namespace sentry {

void SentryMetrics::count(const String &p_name, int64_t p_value, const Dictionary &p_attributes) {
	ERR_FAIL_COND_MSG(p_name.is_empty(), "SentryMetrics.count(): metric name must not be empty.");
	INTERNAL_SDK()->metrics_add_count(p_name, p_value, p_attributes);
}

void SentryMetrics::gauge(const String &p_name, double p_value, const String &p_unit, const Dictionary &p_attributes) {
	ERR_FAIL_COND_MSG(p_name.is_empty(), "SentryMetrics.gauge(): metric name must not be empty.");
	INTERNAL_SDK()->metrics_add_gauge(p_name, p_value, p_unit, p_attributes);
}

void SentryMetrics::distribution(const String &p_name, double p_value, const String &p_unit, const Dictionary &p_attributes) {
	ERR_FAIL_COND_MSG(p_name.is_empty(), "SentryMetrics.distribution(): metric name must not be empty.");
	INTERNAL_SDK()->metrics_add_distribution(p_name, p_value, p_unit, p_attributes);
}

void SentryMetrics::_bind_methods() {
	ClassDB::bind_method(D_METHOD("count", "name", "value", "attributes"), &SentryMetrics::count, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("gauge", "name", "value", "unit", "attributes"), &SentryMetrics::gauge, DEFVAL(String()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("distribution", "name", "value", "unit", "attributes"), &SentryMetrics::distribution, DEFVAL(String()), DEFVAL(Dictionary()));
}

} // namespace sentry
