#include "dotnet_before_send_processor.h"

#include "sentry/dotnet/csharp_interop.h"

namespace sentry::dotnet {

Ref<SentryEvent> DotnetBeforeSendProcessor::process_event(const Ref<SentryEvent> &p_event) {
	// TODO: Determine when crash events can be safely processed in the managed layer. Skipped for now.
	if (p_event.is_null() || p_event->is_crash()) {
		return p_event;
	}
	if (!process_event_in_managed_layer(p_event)) {
		// Event was discarded by the .NET callback.
		return nullptr;
	}
	return p_event;
}

} // namespace sentry::dotnet
