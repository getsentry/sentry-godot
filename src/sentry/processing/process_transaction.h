#pragma once

#include "sentry/sentry_event.h"

namespace sentry {

Ref<SentryEvent> process_transaction(const Ref<SentryEvent> &p_transaction);

} //namespace sentry
