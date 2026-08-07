#pragma once

#include "sentry/level.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_scope_impl.h"
#include "sentry/sentry_user.h"
#include "sentry/util/thread_guard.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry {

// Godot-exported representation of a Sentry scope.
// Platform-specific behavior is provided by SentryScopeImpl subclasses.
class SentryScope : public RefCounted {
	GDCLASS(SentryScope, RefCounted);

private:
	SentryScopeImpl *_impl;

	SENTRY_THREAD_OWNER;

protected:
	static void _bind_methods();

public:
	void set_context(const String &p_key, const Dictionary &p_value);
	void set_tag(const String &p_key, const String &p_value);
	void set_user(const Ref<SentryUser> &p_user);
	void set_level(sentry::Level p_level);
	void set_fingerprint(const PackedStringArray &p_fingerprint);
	void set_attribute(const String &p_name, const Variant &p_value);
	void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb);
	void add_attachment(const Ref<SentryAttachment> &p_attachment);
	void clear();

	Ref<SentryScope> clone() const;

	SentryScopeImpl *get_implementation() const { return _impl; }

	SentryScope();
	SentryScope(SentryScopeImpl *p_impl);
	~SentryScope();
};

} //namespace sentry
