#include "process_event.h"

#include "sentry/contexts.h"
#include "sentry/processing/sentry_event_processor.h"
#include "sentry/sentry_options.h"
#include "sentry/util/print.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>

namespace sentry {

Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) {
	if (p_event.is_null()) {
		sentry::util::print_error("attempted to process a null event");
		return nullptr;
	}

	sentry::util::print_debug("processing event ", p_event->get_id());

	Ref<SentryEvent> event = p_event;

	// Inject contexts
	HashMap<String, Dictionary> contexts = sentry::contexts::make_event_contexts();
	for (const auto &kv : contexts) {
		event->merge_context(kv.key, kv.value);
	}

	// Event processors
	for (const Ref<SentryEventProcessor> &processor : SentryOptions::get_singleton()->get_event_processors()) {
		event = processor->process_event(event);
		if (event.is_null()) {
			return event;
		} else if (event != p_event) {
			sentry::util::print_error("event processor returned a different event object – discarding processor result");
			event = p_event; // Reset to original event
		}
	}

	// Before send callback
	if (const Callable &before_send = SentryOptions::get_singleton()->get_before_send(); before_send.is_valid()) {
		event = before_send.call(event);

		if (event.is_valid() && event != p_event) {
			static bool first_print = true;
			if (unlikely(first_print)) {
				// Note: Only push error once to avoid infinite feedback loop.
				ERR_PRINT("Sentry: before_send callback must return the same event object or null.");
				first_print = false;
			} else {
				sentry::util::print_error("before_send callback must return the same event object or null.");
			}
			return p_event;
		}

		if (event.is_valid()) {
			sentry::util::print_debug("before_send processed ", p_event->get_id());
		} else {
			sentry::util::print_debug("before_send discarded ", p_event->get_id());
		}
	}

	return event;
}

} // namespace sentry
