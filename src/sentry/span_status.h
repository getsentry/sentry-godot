#pragma once

namespace sentry {

// Represents the outcome of a span.
// In the public API, it is exposed as SentrySpan.SpanStatus enum.
enum SpanStatus {
	SPAN_UNSET = -1,
	SPAN_OK = 0,
	SPAN_ERROR = 1,
};

} // namespace sentry
