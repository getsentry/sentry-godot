#include "sentry_span_impl.h"

#include "sentry/disabled/disabled_span.h"

namespace sentry {

SentrySpanImpl *SentrySpanImpl::create_noop() {
	return memnew(DisabledSpan);
}

} //namespace sentry
