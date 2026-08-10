#pragma once

namespace sentry {

// Represents the outcome of a span.
// In the public API, it is exposed as SentrySpan.SpanStatus enum.
// Values match OpenTelemetry's SpanStatusCode, as sentry-javascript does.
enum SpanStatus {
	SPAN_STATUS_UNSET = 0,
	SPAN_STATUS_OK = 1,
	SPAN_STATUS_ERROR = 2,
};

} // namespace sentry
