#ifndef SENTRY_EVENT_PROCESSOR_H
#define SENTRY_EVENT_PROCESSOR_H

#include "sentry/sentry_event.h"

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

// Base class for processing Sentry events before they are sent to the server.
// Implementations can modify, or discard events by returning null.
class SentryEventProcessor : public RefCounted {
	GDCLASS(SentryEventProcessor, RefCounted);

protected:
	static void _bind_methods();

public:
	// Returns the same event (potentially modified) or null to discard it.
	virtual Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) { return p_event; }

	virtual ~SentryEventProcessor() = default;
};

} // namespace sentry

#endif // SENTRY_EVENT_PROCESSOR_H
