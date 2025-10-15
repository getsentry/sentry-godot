#pragma once

#include "sentry/sentry_log.h"

namespace sentry {

// Process log entries in before_send_log callback.
Ref<SentryLog> process_log(const Ref<SentryLog> &p_log);

} //namespace sentry
