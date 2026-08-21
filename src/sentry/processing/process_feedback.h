#pragma once

#include "sentry/sentry_event.h"

namespace sentry {

// Processes prepared user feedback events by applying configured processors,
// and running `before_send_feedback` callback before sending to Sentry.
Ref<SentryEvent> process_feedback(const Ref<SentryEvent> &p_event);

} //namespace sentry
