#pragma once

#include "cocoa_includes.h"
#include "sentry/sentry_span_impl.h"

namespace sentry::cocoa {

class CocoaSpan : public SentrySpanImpl {
	SENTRY_CASTABLE(CocoaSpan, SentrySpanImpl);

private:
	SentryObjCSpan *_span;

	void _apply_attributes(const Dictionary &p_attributes);

public:
	_FORCE_INLINE_ SentryObjCSpan *get_cocoa_span() const { return _span; }

	static SentrySpanImpl *start_root(const String &p_name, const Dictionary &p_attributes);
	virtual SentrySpanImpl *start_child(const String &p_name, const Dictionary &p_attributes) override;
	virtual void set_attribute(const String &p_key, const Variant &p_value) override;
	virtual void set_status(SpanStatus p_status) override;
	virtual void end() override;
	virtual PackedStringArray get_trace_headers() override;

	CocoaSpan(SentryObjCSpan *p_span, const Dictionary &p_attributes);
	virtual ~CocoaSpan() override;
};

} //namespace sentry::cocoa
