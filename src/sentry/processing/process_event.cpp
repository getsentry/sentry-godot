#include "process_event.h"

#include "sentry/contexts.h"
#include "sentry/engine_lifecycle/engine_lifecycle.h"
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

using namespace sentry;

namespace {

Ref<SentryEvent> _process_during_shutdown(const Ref<SentryEvent> &p_event) {
	// During shutdown, the scene tree, GDScript runtime, and user Callables
	// are being released. Skip contexts, event processors, and before_send.
	sentry::logging::print_no_logger(
			sentry::LEVEL_DEBUG,
			"Capturing event during shutdown (reduced processing) ",
			p_event->get_id());
	return p_event;
}

} // unnamed namespace

namespace sentry {

Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) {
	// Track processing before touching Godot machinery so shutdown waits for it to complete.
	sentry::engine_lifecycle::ProcessingSection processing_section;

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

	if (sentry::engine_lifecycle::is_shutting_down()) {
		// Better not touch Godot machinery during shutdown. Skip processing.
		return _process_during_shutdown(p_event);
	}

	sentry::logging::print_debug("Processing event ", p_event->get_id());

	Ref<SentryEvent> event = p_event;

	// Inject contexts.
	// NOTE: On Cocoa/Android, crash reports are processed after app restart,
	// so we skip enrichment to avoid attaching stale data from the current session.
	// Native SDK processes crashes in the same session, so enrichment is safe.
#if defined(SDK_COCOA) || defined(SDK_ANDROID)
	constexpr bool enrich_crashes = false;
#else
	constexpr bool enrich_crashes = true;
#endif
	if (enrich_crashes || !p_event->is_crash()) {
		HashMap<String, Dictionary> contexts = sentry::contexts::make_event_contexts();
		for (const auto &kv : contexts) {
			event->merge_context(kv.key, kv.value);
		}
	}

	// Patch device_type into the device context on Cocoa and Android.
	// These platform SDKs own the device context but don't include device_type,
	// so we inject it per-event here. This covers both crash and non-crash events.
#if defined(SDK_COCOA) || defined(SDK_ANDROID)
	event->merge_context("device", sentry::contexts::make_device_context_patch());
#endif

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
