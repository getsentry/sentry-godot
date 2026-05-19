#pragma once

#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_user.h"

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

class SentryScopeObserver : public RefCounted {
	GDCLASS(SentryScopeObserver, RefCounted);

protected:
	static void _bind_methods() {}

public:
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {}

	virtual void set_tag(const String &p_key, const String &p_value) {}
	virtual void remove_tag(const String &p_key) {}

	virtual void set_user(const Ref<SentryUser> &p_user) {}
	virtual void remove_user() {}

	virtual ~SentryScopeObserver() = default;
};

} //namespace sentry
