#pragma once

#include "sentry.h"
#include "sentry/sentry_span_impl.h"

namespace sentry::native {

class NativeSpan : public SentrySpanImpl {
	SENTRY_CASTABLE(NativeSpan, SentrySpanImpl);

private:
	// Native emulates the span-first API, until the day span-first is actually supported.
	// Root spans are backed by transactions, while child spans use native spans.
	sentry_transaction_t *_transaction = nullptr;
	sentry_span_t *_span = nullptr;

	void _apply_attributes(const Dictionary &p_attributes);

	NativeSpan(sentry_transaction_t *p_transaction, const Dictionary &p_attributes);
	NativeSpan(sentry_span_t *p_span, const Dictionary &p_attributes);

public:
	static SentrySpanImpl *start_root(const String &p_name, const Dictionary &p_attributes);
	virtual SentrySpanImpl *start_child(const String &p_name, const Dictionary &p_attributes) override;

	virtual void set_attribute(const String &p_key, const Variant &p_value) override;
	virtual void set_status(SpanStatus p_status) override;

	virtual void end() override;

	virtual PackedStringArray get_trace_headers() override;

	void bind_to_scope(sentry_scope_t *p_scope);

	virtual ~NativeSpan() override;
};

} //namespace sentry::native
