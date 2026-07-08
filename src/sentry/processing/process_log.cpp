#include "process_log.h"

#include "sentry/engine_lifecycle/engine_lifecycle.h"
#include "sentry/sentry_sdk.h"

namespace sentry {

Ref<SentryLog> process_log(const Ref<SentryLog> &p_log) {
	// Track processing before invoking the user Callable so shutdown waits for it to complete.
	sentry::engine_lifecycle::ProcessingSection processing_section;

	const Callable &before_send_log = SENTRY_OPTIONS()->get_before_send_log();
	if (before_send_log.is_null()) {
		return p_log;
	}

	Ref<SentryLog> processed = before_send_log.call(p_log);
	if (processed.is_null()) {
		return nullptr;
	}

	return p_log;
}

} //namespace sentry
