#ifndef SENTRY_PROCESS_EVENT_H
#define SENTRY_PROCESS_EVENT_H

#include "sentry_event.h"

namespace sentry {

// Processes events by adding contexts, applying configured processors,
// and running `before_send` callback before sending to Sentry.
Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event);

} //namespace sentry

#endif // SENTRY_PROCESS_EVENT_H
