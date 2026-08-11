#pragma once

#include "sentry/sentry_span_impl.h"
#include "sentry/span_status.h"
#include "sentry/util/thread_guard.h"

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

// Godot-exported representation of a Sentry span.
// Platform-specific behavior is provided by SentrySpanImpl subclasses.
class SentrySpan : public RefCounted {
	GDCLASS(SentrySpan, RefCounted);

public:
	// SentrySpan.SpanStatus is defined in sentry/span_status.h.
	// Godot extensions can't expose global enums; they must belong to a class.
	// This alias avoids circular dependencies with headers that use SpanStatus.
	using SpanStatus = sentry::SpanStatus;

private:
	SentrySpanImpl *_impl;

	// The span this one displaced when it was bound to a scope, assigned by SentrySDK.
	// Scopes resolve their slot through this chain, so it must outlive this span's end().
	Ref<SentrySpan> _previous;

	bool _ended = false;

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

	// *** Not exposed in the public API

	Ref<SentrySpan> start_child(const String &p_name, const Dictionary &p_attributes);

	_FORCE_INLINE_ bool is_ended() const { return _ended; }
	_FORCE_INLINE_ void set_previous(const Ref<SentrySpan> &p_span) { _previous = p_span; }
	_FORCE_INLINE_ Ref<SentrySpan> get_previous() const { return _previous; }

	SentrySpanImpl *get_implementation() const { return _impl; }

	SentrySpan();
	SentrySpan(const String &p_name, const Dictionary &p_attributes);
	SentrySpan(SentrySpanImpl *p_impl);
	~SentrySpan();
};

} // namespace sentry

VARIANT_ENUM_CAST(sentry::SentrySpan::SpanStatus);
