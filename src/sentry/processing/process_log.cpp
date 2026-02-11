#include "process_log.h"

#include "sentry/sentry_sdk.h"

namespace sentry {

Ref<SentryLog> process_log(const Ref<SentryLog> &p_log) {
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
