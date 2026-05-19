#pragma once

#include "sentry/sentry_scope_observer.h"

#include <cstdint>
#include <godot_cpp/core/defs.hpp>

namespace sentry::dotnet {

class DotnetScopeObserver : public SentryScopeObserver {
	GDCLASS(DotnetScopeObserver, SentryScopeObserver);

protected:
	static void _bind_methods() {}

public:
	// Prevent feedback loops when syncing scope changes across layers.
	// Incremented by DotnetScopeObserver for outbound updates and by managed callers in csharp_interop.cpp for inbound updates.
	class SyncGuard {
	private:
		static thread_local uint32_t _depth;

	public:
		_FORCE_INLINE_ static bool is_syncing() { return _depth > 0; }

		SyncGuard() { ++_depth; }
		~SyncGuard() { --_depth; }

		SyncGuard(const SyncGuard &) = delete;
		SyncGuard &operator=(const SyncGuard &) = delete;
		SyncGuard(SyncGuard &&) = delete;
		SyncGuard &operator=(SyncGuard &&) = delete;
	};

	virtual void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) override;

	virtual void set_tag(const String &p_key, const String &p_value) override;
	virtual void remove_tag(const String &p_key) override;

	virtual void set_user(const Ref<SentryUser> &p_user) override;
	virtual void remove_user() override;
};

} //namespace sentry::dotnet
