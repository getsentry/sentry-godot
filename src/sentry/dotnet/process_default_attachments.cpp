#include "process_default_attachments.h"

#include "sentry/disabled/disabled_event.h"
#include "sentry/logging/print.h"
#include "sentry/sentry_options.h"
#include "sentry/sentry_sdk.h"

namespace sentry::dotnet {

void process_default_attachments() {
	// Use a dummy event to process default attachments.
	// TODO: Replace this with the full .NET event pipeline when it lands.
	sentry::logging::print_debug("Processing default attachments...");
	Ref<DisabledEvent> dummy_event;
	dummy_event.instantiate();
	for (const Ref<SentryEventProcessor> &processor : SENTRY_OPTIONS()->get_event_processors()) {
		dummy_event = processor->process_event(dummy_event);
		if (dummy_event.is_null()) {
			return;
		}
	}
}

} //namespace sentry::dotnet
