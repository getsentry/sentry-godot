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
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) = 0;

	virtual void set_tag(const String &p_key, const String &p_value) = 0;
	virtual void remove_tag(const String &p_key) = 0;

	virtual void set_user(const Ref<SentryUser> &p_user) = 0;
	virtual void remove_user() = 0;

	virtual ~SentryScopeObserver() = default;
};

} //namespace sentry
