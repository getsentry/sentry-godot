#pragma once

#include "sentry/castable.h"
#include "sentry/span_status.h"

#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry {

// Base class for Sentry span implementations; see Godot-facing SentrySpan.
// Kept as a pure C++ class instead of a Godot class to avoid ClassDB
// registration and reduce overhead.
// Lifetime governed by SentrySpan.
class SentrySpanImpl : public Castable {
	SENTRY_CASTABLE(SentrySpanImpl, Castable);

public:
	virtual void set_attribute(const String &p_key, const Variant &p_value) = 0;
	virtual Variant get_attribute(const String &p_key) const = 0;
	virtual Dictionary get_attributes() const = 0;
	virtual void set_status(SpanStatus p_status) = 0;
	virtual SpanStatus get_status() const = 0;
	virtual void set_name(const String &p_name) = 0;
	virtual String get_name() const = 0;
	virtual void end() = 0;

	virtual ~SentrySpanImpl() = default;
};

} //namespace sentry
