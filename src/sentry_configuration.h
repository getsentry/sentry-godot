#ifndef SENTRY_CONFIGURATION_H
#define SENTRY_CONFIGURATION_H

#include "sentry_options.h"

#include <godot_cpp/classes/node.hpp>
// #include "godot_cpp/classes/ref_counted.hpp"
#include <godot_cpp/core/gdvirtual.gen.inc>

using namespace godot;

class SentryConfiguration : public Node {
	GDCLASS(SentryConfiguration, Node);

protected:
	// friend class SentrySDK;

	static void _bind_methods();
	void _notification(int p_what);

	GDVIRTUAL1(_initialize, Ref<SentryOptions>);

	// SentrySDK should call this method upon initialization.
	void _call_initialize(const Ref<SentryOptions> &p_options);

public:
	SentryConfiguration() {}
};

#endif // SENTRY_CONFIGURATION_H
