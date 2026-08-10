#pragma once

#include "sentry.h"
#include "sentry/sentry_span_impl.h"

namespace sentry::native {

// Stub: spans are not implemented on this platform yet.
class NativeSpan : public SentrySpanImpl {
	SENTRY_CASTABLE(NativeSpan, SentrySpanImpl);

private:
	// Native emulates the span-first API, until the day span-first is actually supported.
	// Root spans are backed by transactions, while child spans use native spans.
	union {
		sentry_transaction_t *transaction;
		sentry_span_t *span;
	} _data;
	bool _is_transaction = true;

public:
	virtual void set_attribute(const String &p_key, const Variant &p_value) override;
	virtual Variant get_attribute(const String &p_key) const override;
	virtual Dictionary get_attributes() const override;

	virtual void set_status(SpanStatus p_status) override;
	virtual SpanStatus get_status() const override;

	virtual void set_name(const String &p_name) override;
	virtual String get_name() const override;

	virtual void end() override;

	virtual SentrySpanImpl *start_child(const String &p_name) override;

	NativeSpan();
	virtual ~NativeSpan() override;
};

} //namespace sentry::native
