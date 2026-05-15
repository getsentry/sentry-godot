#include "dotnet_scope_observer.h"

#include "sentry/dotnet/csharp_interop.h"

namespace sentry::dotnet {

thread_local uint32_t DotnetScopeObserver::SyncGuard::_depth = 0;

void DotnetScopeObserver::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	if (SyncGuard::is_syncing()) {
		return;
	}
	SyncGuard guard;
	sentry::dotnet::add_breadcrumb(p_breadcrumb);
}

void DotnetScopeObserver::set_tag(const String &p_key, const String &p_value) {
	if (SyncGuard::is_syncing()) {
		return;
	}
	SyncGuard guard;
	sentry::dotnet::set_tag(p_key, p_value);
}

void DotnetScopeObserver::remove_tag(const String &p_key) {
	if (SyncGuard::is_syncing()) {
		return;
	}
	SyncGuard guard;
	sentry::dotnet::remove_tag(p_key);
}

void DotnetScopeObserver::set_user(const Ref<SentryUser> &p_user) {
	if (SyncGuard::is_syncing()) {
		return;
	}
	SyncGuard guard;
	sentry::dotnet::set_user(p_user);
}

void DotnetScopeObserver::remove_user() {
	if (SyncGuard::is_syncing()) {
		return;
	}
	SyncGuard guard;
	sentry::dotnet::remove_user();
}

} //namespace sentry::dotnet
