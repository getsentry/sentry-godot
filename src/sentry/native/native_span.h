#pragma once

#include "sentry/sentry_span_impl.h"

namespace sentry::native {

// Stub: spans are not implemented on this platform yet.
class NativeSpan : public SentrySpanImpl {
	SENTRY_CASTABLE(NativeSpan, SentrySpanImpl);

public:
	virtual void set_attribute(const String &p_key, const Variant &p_value) override;
	virtual Variant get_attribute(const String &p_key) const override;
	virtual Dictionary get_attributes() const override;

	virtual void set_status(SpanStatus p_status) override;
	virtual SpanStatus get_status() const override;

	virtual void set_name(const String &p_name) override;
	virtual String get_name() const override;

	virtual void end() override;

	NativeSpan();
	virtual ~NativeSpan() override;
};

} //namespace sentry::native
