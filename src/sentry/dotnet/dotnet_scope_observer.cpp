#include "dotnet_scope_observer.h"

#include "sentry/dotnet/csharp_interop.h"

namespace sentry::dotnet {

void DotnetScopeObserver::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	sentry::dotnet::add_breadcrumb(p_breadcrumb);
}

void DotnetScopeObserver::set_tag(const String &p_key, const String &p_value) {
	sentry::dotnet::set_tag(p_key, p_value);
}

void DotnetScopeObserver::remove_tag(const String &p_key) {
	sentry::dotnet::remove_tag(p_key);
}

void DotnetScopeObserver::set_user(const Ref<SentryUser> &p_user) {
	sentry::dotnet::set_user(p_user);
}

void DotnetScopeObserver::remove_user() {
	sentry::dotnet::remove_user();
}

} //namespace sentry::dotnet
