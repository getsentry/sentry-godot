#pragma once

#include "sentry/sentry_span_impl.h"

namespace sentry::android {

// Android emulates the span-first API, until the day span-first is actually supported.
// Root spans are backed by transactions, while child spans use sentry-java spans.
class AndroidSpan : public SentrySpanImpl {
	SENTRY_CASTABLE(AndroidSpan, SentrySpanImpl);

private:
	Object *_android_plugin = nullptr;
	int32_t _handle = 0;

	void _apply_attributes(const Dictionary &p_attributes);

	AndroidSpan(Object *p_android_plugin, int32_t p_handle, const Dictionary &p_attributes);

public:
	static SentrySpanImpl *start_root(Object *p_android_plugin, const String &p_name, const Dictionary &p_attributes);

	virtual SentrySpanImpl *start_child(const String &p_name, const Dictionary &p_attributes) override;

	int32_t get_handle() const { return _handle; }

	virtual void set_attribute(const String &p_key, const Variant &p_value) override;
	virtual void set_status(SpanStatus p_status) override;
	virtual void end() override;

	virtual PackedStringArray get_trace_headers() override;

	virtual ~AndroidSpan() override;
};

} //namespace sentry::android
