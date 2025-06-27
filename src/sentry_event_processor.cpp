#include "sentry_event_processor.h"

void SentryEventProcessor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("process_event"), &SentryEventProcessor::process_event);
}
