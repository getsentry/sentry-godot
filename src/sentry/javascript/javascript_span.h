#pragma once

#include "sentry/javascript/javascript_interop.h"
#include "sentry/sentry_span_impl.h"

namespace sentry::javascript {

// Backed by a JS Span object, which is guaranteed to be valid: creation failures yield a DisabledSpan instead.
// See JavaScriptSDK::create_span().
class JavaScriptSpan : public SentrySpanImpl {
	SENTRY_CASTABLE(JavaScriptSpan, SentrySpanImpl);

private:
	JSObjectPtr js_obj;

public:
	_FORCE_INLINE_ JSObjectPtr get_js_object() const { return js_obj; }

	virtual void set_attribute(const String &p_key, const Variant &p_value) override;
	virtual void set_status(SpanStatus p_status) override;
	virtual void end() override;

	virtual SentrySpanImpl *start_child(const String &p_name, const Dictionary &p_attributes) override;

	explicit JavaScriptSpan(const JSObjectPtr &p_js_span_object);
	JavaScriptSpan() = delete;
};

// Starts a JavaScript span, parented to p_parent when provided; otherwise starts a root-level span.
SentrySpanImpl *start_js_span(const String &p_name, const Dictionary &p_attributes, const JSObjectPtr &p_parent);

} //namespace sentry::javascript
