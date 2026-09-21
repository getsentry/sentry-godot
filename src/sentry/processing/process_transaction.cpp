#include "process_transaction.h"

#include "sentry/logging/print.h"
#include "sentry/sentry_sdk.h"
#include "sentry/util/recursion_guard.h"

namespace sentry {

Ref<SentryEvent> process_transaction(const Ref<SentryEvent> &p_transaction) {
	static thread_local uint32_t processing_depth = 0;
	sentry::util::RecursionGuard guard{ &processing_depth };
	if (!guard.should_proceed()) {
		sentry::logging::print_no_logger(sentry::LEVEL_WARNING, "Skipping transaction processing for a secondary transaction triggered while processing another transaction. Transaction will still be sent.");
		return p_transaction;
	}

	if (p_transaction.is_null()) {
		sentry::logging::print_error("Attempted to process a null transaction");
		return nullptr;
	}

	Ref<SentryEvent> transaction = p_transaction;
	if (const Callable &before_send_transaction = SENTRY_OPTIONS()->get_before_send_transaction(); before_send_transaction.is_valid()) {
		transaction = before_send_transaction.call(transaction);

		if (transaction.is_valid() && transaction != p_transaction) {
			ERR_PRINT_ONCE("Sentry: before_send_transaction callback must return the same transaction object or null.");
			return p_transaction;
		}

		if (transaction.is_valid()) {
			sentry::logging::print_debug("before_send_transaction processed transaction");
		} else {
			sentry::logging::print_debug("before_send_transaction discarded transaction");
		}
	}

	return transaction;
}

} //namespace sentry
