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

	// Finishing hands the handle over to sentry-native, which frees it.
	_FORCE_INLINE_ bool _is_live() const { return _transaction || _span; }

	void _apply_attributes(const Dictionary &p_attributes);

public:
	virtual void set_attribute(const String &p_key, const Variant &p_value) override;
	virtual void set_status(SpanStatus p_status) override;

	virtual void end() override;

	virtual SentrySpanImpl *start_child(const String &p_name, const Dictionary &p_attributes) override;

	NativeSpan() = delete;
	NativeSpan(const String &p_name, const Dictionary &p_attributes);
	NativeSpan(sentry_span_t *p_span, const Dictionary &p_attributes);
	virtual ~NativeSpan() override;
};

} //namespace sentry::native
