#include "dotnet_scope_observer.h"

#include "sentry/dotnet/csharp_interop.h"

namespace sentry::dotnet {

void DotnetScopeObserver::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	if (p_breadcrumb.is_valid()) {
		sentry::dotnet::add_breadcrumb(p_breadcrumb);
	}
}

void DotnetScopeObserver::set_tag(const String &p_key, const String &p_value) {
}

void DotnetScopeObserver::remove_tag(const String &p_key) {
}

void DotnetScopeObserver::set_user(const Ref<SentryUser> &p_user) {
}

void DotnetScopeObserver::remove_user() {
}

} //namespace sentry::dotnet
