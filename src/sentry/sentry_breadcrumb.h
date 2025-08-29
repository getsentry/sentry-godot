#ifndef SENTRY_BREADCRUMB_H
#define SENTRY_BREADCRUMB_H

#include "sentry/level.h"

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

// Represents breadcrumbs in the public API.
class SentryBreadcrumb : public RefCounted {
	GDCLASS(SentryBreadcrumb, RefCounted);

protected:
	static void _bind_methods();

public:
	static Ref<SentryBreadcrumb> debug(const String &p_message);
	static Ref<SentryBreadcrumb> info(const String &p_message);
	static Ref<SentryBreadcrumb> error(const String &p_message);
	static Ref<SentryBreadcrumb> query(const String &p_message);
	static Ref<SentryBreadcrumb> user(const String &p_category, const String &p_message);

	virtual void set_message(const String &p_message) = 0;
	virtual String get_message() const = 0;

	virtual void set_category(const String &p_category) = 0;
	virtual String get_category() const = 0;

	virtual void set_level(sentry::Level p_level) = 0;
	virtual sentry::Level get_level() const = 0;

	virtual void set_type(const String &p_type) = 0;
	virtual String get_type() const = 0;

	virtual void set_data(const Dictionary &p_data) = 0;

	virtual ~SentryBreadcrumb() = default;
};

} //namespace sentry

#endif // SENTRY_BREADCRUMB_H
