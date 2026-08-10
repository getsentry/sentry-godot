#pragma once

#include "sentry/util/thread_guard.h"

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

class SentrySpan : public RefCounted {
	GDCLASS(SentrySpan, RefCounted);

public:
	enum SpanStatus {
		SPAN_UNSET = -1,
		SPAN_OK = 0,
		SPAN_ERROR = 1,
	};

private:
	// SentrySpanImpl *_impl;

	SENTRY_THREAD_OWNER;

protected:
	static void _bind_methods();

public:
	// Returns a sentinel value that indicates an unassigned span.
	static Ref<SentrySpan> unassigned();

	void set_attribute(const String &p_key, const Variant &p_value);
	void set_attributes(const Dictionary &p_attributes);
	Variant get_attribute(const String &p_key) const;
	Dictionary get_attributes() const;

	void set_status(SpanStatus p_status);
	SpanStatus get_status() const;

	void set_name(const String &p_name);
	String get_name() const;

	void end();
};

} // namespace sentry
