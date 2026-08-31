#pragma once

namespace sentry {

// Represents the outcome of a span.
// In the public API, it is exposed as SentrySpan.SpanStatus enum.
enum SpanStatus {
	SPAN_STATUS_OK = 0,
	SPAN_STATUS_ERROR = 1,
};

} // namespace sentry
