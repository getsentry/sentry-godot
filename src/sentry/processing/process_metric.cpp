#include "process_metric.h"

#include "sentry/engine_lifecycle/engine_lifecycle.h"
#include "sentry/sentry_sdk.h"

namespace sentry {

Ref<SentryMetric> process_metric(const Ref<SentryMetric> &p_metric) {
	if (sentry::engine_lifecycle::is_shutting_down()) {
		// Shouldn't touch Godot machinery during shutdown.
		return p_metric;
	}

	// Track processing before touching Godot machinery so shutdown waits for it to complete.
	sentry::engine_lifecycle::ProcessingSection processing_section;

	const Callable &before_send_metric = SENTRY_OPTIONS()->get_before_send_metric();
	if (before_send_metric.is_null()) {
		return p_metric;
	}

	Ref<SentryMetric> processed = before_send_metric.call(p_metric);
	if (processed == p_metric || processed.is_null()) {
		return processed;
	} else {
		ERR_PRINT_ONCE("Sentry: before_send_metric callback must return the same metric object or null.");
		return p_metric;
	}
}

} //namespace sentry
