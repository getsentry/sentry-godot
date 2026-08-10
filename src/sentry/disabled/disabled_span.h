#pragma once

#include "sentry/sentry_span_impl.h"

namespace sentry {

// Span implementation that is used when the SDK is disabled.
// Nothing is sent, but values are kept so that getters don't contradict setters.
class DisabledSpan : public SentrySpanImpl {
	SENTRY_CASTABLE(DisabledSpan, SentrySpanImpl);

private:
	Dictionary attributes;
	SpanStatus status = SPAN_STATUS_UNSET;
	String name;

public:
	virtual void set_attribute(const String &p_key, const Variant &p_value) override { attributes[p_key] = p_value; }
	virtual Variant get_attribute(const String &p_key) const override { return attributes.get(p_key, Variant()); }
	virtual Dictionary get_attributes() const override { return attributes.duplicate(); }

	virtual void set_status(SpanStatus p_status) override { status = p_status; }
	virtual SpanStatus get_status() const override { return status; }

	virtual void set_name(const String &p_name) override { name = p_name; }
	virtual String get_name() const override { return name; }

	virtual void end() override {}

	virtual SentrySpanImpl *start_child(const String &p_name, const Dictionary &p_attributes) override { return memnew(DisabledSpan); }
};

} //namespace sentry
