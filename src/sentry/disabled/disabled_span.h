#pragma once

#include "sentry/sentry_span_impl.h"

namespace sentry {

// Span implementation that is used when the SDK is disabled.
class DisabledSpan : public SentrySpanImpl {
	SENTRY_CASTABLE(DisabledSpan, SentrySpanImpl);

public:
	virtual void set_attribute(const String &p_key, const Variant &p_value) override {}
	virtual void set_status(SpanStatus p_status) override {}
	virtual void end() override {}

	virtual SentrySpanImpl *start_child(const String &p_name, const Dictionary &p_attributes) override { return memnew(DisabledSpan); }
};

} //namespace sentry
