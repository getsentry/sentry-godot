#include "sentry_span.h"

namespace sentry {

Ref<SentrySpan> SentrySpan::unassigned() {
	static Ref<SentrySpan> sentinel;
	if (sentinel.is_null()) {
		sentinel.instantiate();
	}
	return sentinel;
}

} // namespace sentry
