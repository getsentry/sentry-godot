#pragma once

#include "sentry/sentry_scope_observer.h"

namespace sentry::dotnet {

class DotnetScopeObserver : public SentryScopeObserver {
	GDCLASS(DotnetScopeObserver, SentryScopeObserver);

protected:
	static void _bind_methods() {}

public:
	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) override;

	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void remove_tag(const String &p_key) override;

	virtual void set_user(const Ref<SentryUser> &p_user) override;
	virtual void remove_user() override;
};

} //namespace sentry::dotnet
