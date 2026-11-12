#pragma once

#include "sentry/castable.h"
#include "sentry/sentry_breadcrumb.h"

namespace sentry {

// Lifetime governed by the Godot-facing SentryBreadcrumb.
class SentryBreadcrumbImpl : public Castable {
	SENTRY_CASTABLE(SentryBreadcrumbImpl, Castable);

public:
	virtual void set_message(const String &p_message) = 0;
	virtual String get_message() const = 0;

	virtual void set_category(const String &p_category) = 0;
	virtual String get_category() const = 0;

	virtual void set_level(sentry::Level p_level) = 0;
	virtual sentry::Level get_level() const = 0;

	virtual void set_type(const String &p_type) = 0;
	virtual String get_type() const = 0;

	virtual void set_data(const Dictionary &p_data) = 0;

	virtual Ref<SentryTimestamp> get_timestamp() = 0;

	SentryBreadcrumbImpl() = default;
	SentryBreadcrumbImpl(const SentryBreadcrumbImpl &) = delete;
	SentryBreadcrumbImpl &operator=(const SentryBreadcrumbImpl &) = delete;

	virtual ~SentryBreadcrumbImpl() = default;
};

} // namespace sentry
