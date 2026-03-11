#pragma once

#include "sentry/sentry_metric.h"

namespace sentry {

Ref<SentryMetric> process_metric(const Ref<SentryMetric> &p_metric);

} // namespace sentry
