#ifndef SENTRY_PROCESS_EVENT_H
#define SENTRY_PROCESS_EVENT_H

#include "sentry_event.h"

namespace sentry {

Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event);

}

#endif // SENTRY_PROCESS_EVENT_H
