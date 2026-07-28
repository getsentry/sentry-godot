#pragma once

#include "sentry/level.h"
#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_user.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry {

// Base class for Sentry scope implementations; see Godot-facing SentryScope.
// Splitting the implementation from SentryScope avoids a factory method
// (i.e. SentryScope.create() or SentrySDK.create_scope()).
// Kept as a pure C++ class instead of a Godot class to avoid ClassDB
// registration and reduce overhead.
// Lifetime governed by SentryScope.
class SentryScopeImpl {
public:
	virtual void set_context(const String &p_key, const Dictionary &p_value) = 0;
	virtual void set_tag(const String &p_key, const String &p_value) = 0;
	virtual void set_user(const Ref<SentryUser> &p_user) = 0;
	virtual void set_level(sentry::Level p_level) = 0;
	virtual void set_fingerprint(const PackedStringArray &p_fingerprint) = 0;
	virtual void set_attribute(const String &p_name, const Variant &p_value) = 0;
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) = 0;
	virtual void clear() = 0;
	virtual SentryScopeImpl *clone() const = 0;

	virtual ~SentryScopeImpl() = default;
};

} //namespace sentry
