#include "enrichment_processor.h"

#include "sentry/contexts.h"

namespace sentry {

Ref<SentryEvent> EnrichmentProcessor::process_event(const Ref<SentryEvent> &p_event) {
	Ref<SentryEvent> event = p_event;

	// NOTE: On Cocoa/Android, crash reports are processed after app restart,
	// so we skip enrichment to avoid attaching stale data from the current session.
	// Native SDK processes crashes in the same session, so enrichment is safe.
#if defined(SDK_COCOA) || defined(SDK_ANDROID)
	constexpr bool enrich_crashes = false;
#else
	constexpr bool enrich_crashes = true;
#endif
	if (enrich_crashes || !event->is_crash()) {
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

	return event;
}

} // namespace sentry
