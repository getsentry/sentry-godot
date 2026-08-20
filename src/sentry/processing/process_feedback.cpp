#include "process_feedback.h"

#include "sentry/dotnet/csharp_interop.h"
#include "sentry/logging/print.h"
#include "sentry/processing/sentry_event_processor.h"
#include "sentry/sentry_sdk.h"
#include "sentry/util/recursion_guard.h"

namespace sentry {

Ref<SentryEvent> process_feedback(const Ref<SentryEvent> &p_event) {
	static thread_local uint32_t processing_depth = 0;
	sentry::util::RecursionGuard guard{ &processing_depth };
	if (!guard.should_proceed()) {
		// Avoid logger in case it's a triggering path for the cascaded error.
		sentry::logging::print_no_logger(sentry::LEVEL_WARNING, "Skipping feedback processing for secondary feedback triggered while processing another one. Feedback will still be sent.");
		return p_event;
	}

	if (p_event.is_null()) {
		sentry::logging::print_error("Attempted to process a null feedback event");
		return nullptr;
	}

	sentry::logging::print_debug("Processing feedback ", p_event->get_id());

	Ref<SentryEvent> event = p_event;

	for (const Ref<SentryEventProcessor> &processor : SENTRY_OPTIONS()->get_event_processors()) {
		event = processor->process_event(event);
		if (event.is_null()) {
			return event;
		} else if (event != p_event) {
			sentry::logging::print_error("Event processor returned a different event object – discarding processor result");
			event = p_event; // Reset to original event
		}
	}

	// Managed (.NET) before-send-feedback hook.
	if (!sentry::dotnet::process_feedback_in_managed_layer(event)) {
		sentry::logging::print_debug("managed layer discarded feedback ", p_event->get_id());
		return nullptr;
	}

	if (const Callable &before_send_feedback = SENTRY_OPTIONS()->get_before_send_feedback(); before_send_feedback.is_valid()) {
		event = before_send_feedback.call(event);

		if (event.is_valid() && event != p_event) {
			ERR_PRINT_ONCE("Sentry: before_send_feedback callback must return the same event object or null.");
			return p_event;
		}

		if (event.is_valid()) {
			sentry::logging::print_debug("before_send_feedback processed ", p_event->get_id());
		} else {
			sentry::logging::print_debug("before_send_feedback discarded ", p_event->get_id());
		}
	}

	return event;
}

} // namespace sentry
