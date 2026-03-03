#include "process_metric.h"

#include "sentry/sentry_sdk.h"

namespace sentry {

Ref<SentryMetric> process_metric(const Ref<SentryMetric> &p_metric) {
	const Callable &before_send_metric = SENTRY_OPTIONS()->get_experimental()->get_before_send_metric();
	if (before_send_metric.is_null()) {
		return p_metric;
	}

	Ref<SentryMetric> processed = before_send_metric.call(p_metric);
	if (processed.is_null()) {
		return nullptr;
	}

	// Replacing the metric instance isn't supported; always return the original,
	// whether it was modified or not.
	return p_metric;
}

} //namespace sentry
