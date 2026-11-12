#pragma once

#include "sentry/level.h"
#include "sentry/sentry_timestamp.h"

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

class SentryBreadcrumbImpl;

// Represents breadcrumbs in the public API.
class SentryBreadcrumb : public RefCounted {
	GDCLASS(SentryBreadcrumb, RefCounted);

private:
	SentryBreadcrumbImpl *_impl = nullptr;

protected:
	static void _bind_methods();

public:
	static Ref<SentryBreadcrumb> create(const String &p_message = "");

	void set_message(const String &p_message);
	String get_message() const;

	void set_category(const String &p_category);
	String get_category() const;

	void set_level(sentry::Level p_level);
	sentry::Level get_level() const;

	void set_type(const String &p_type);
	String get_type() const;

	void set_data(const Dictionary &p_data);

	Ref<SentryTimestamp> get_timestamp();

	SentryBreadcrumbImpl *get_implementation() const { return _impl; }

	SentryBreadcrumb();
	explicit SentryBreadcrumb(SentryBreadcrumbImpl *p_impl);
	virtual ~SentryBreadcrumb() override;
};

} //namespace sentry
