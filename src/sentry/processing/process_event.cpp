#include "process_event.h"

#include "sentry/dotnet/csharp_interop.h"
#include "sentry/logging/print.h"
#include "sentry/processing/sentry_event_processor.h"
#include "sentry/sentry_sdk.h"
#include "sentry/util/recursion_guard.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>

namespace sentry {

Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) {
	static thread_local uint32_t processing_depth = 0;
	sentry::util::RecursionGuard guard{ &processing_depth };
	if (!guard.should_proceed()) {
		// Avoid logger in case it's a triggering path for the cascaded error.
		sentry::logging::print_no_logger(sentry::LEVEL_WARNING, "Skipping event processing for secondary event triggered while processing another event. Event will still be sent.");
		return p_event;
	}

	if (p_event.is_null()) {
		sentry::logging::print_error("Attempted to process a null event");
		return nullptr;
	}

	sentry::logging::print_debug("Processing event ", p_event->get_id());

	Ref<SentryEvent> event = p_event;

	// Event processors
	for (const Ref<SentryEventProcessor> &processor : SENTRY_OPTIONS()->get_event_processors()) {
		event = processor->process_event(event);
		if (event.is_null()) {
			return event;
		} else if (event != p_event) {
			sentry::logging::print_error("Event processor returned a different event object – discarding processor result");
			event = p_event; // Reset to original event
		}
	}

	// Managed (.NET) before-send hook.
	// TODO: Determine when crash events can be safely processed in the managed layer. Skipped for now.
	if (!event->is_crash() && !sentry::dotnet::process_event_in_managed_layer(event)) {
		sentry::logging::print_debug("managed layer discarded ", p_event->get_id());
		return nullptr;
	}

	// Before send callback
	if (const Callable &before_send = SENTRY_OPTIONS()->get_before_send(); before_send.is_valid()) {
		event = before_send.call(event);

		if (event.is_valid() && event != p_event) {
			static bool first_print = true;
			if (unlikely(first_print)) {
				// Note: Only push error once to avoid infinite feedback loop.
				ERR_PRINT("Sentry: before_send callback must return the same event object or null.");
				first_print = false;
			} else {
				sentry::logging::print_error("before_send callback must return the same event object or null.");
			}
			return p_event;
		}

		if (event.is_valid()) {
			sentry::logging::print_debug("before_send processed ", p_event->get_id());
		} else {
			sentry::logging::print_debug("before_send discarded ", p_event->get_id());
		}
	}

	return event;
}

} // namespace sentry
