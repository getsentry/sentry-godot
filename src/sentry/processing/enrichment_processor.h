#pragma once

#include "sentry/processing/sentry_event_processor.h"

namespace sentry {

// Event processor that injects Godot-specific contexts, such as engine and performance info.
class EnrichmentProcessor : public SentryEventProcessor {
	GDCLASS(EnrichmentProcessor, SentryEventProcessor);

protected:
	static void _bind_methods() {}

public:
	virtual Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) override;
};

} // namespace sentry
