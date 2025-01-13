#ifndef SENTRY_CONFIGURATION_H
#define SENTRY_CONFIGURATION_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

using namespace godot;

class SentryConfiguration : public RefCounted {
	GDCLASS(SentryConfiguration, RefCounted);

protected:
	friend class SentrySDK;

	static void _bind_methods();

	GDVIRTUAL0(_initialize);

	void _call_initialize();
};

#endif // SENTRY_CONFIGURATION_H
