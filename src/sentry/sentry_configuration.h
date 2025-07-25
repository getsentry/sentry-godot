#ifndef SENTRY_CONFIGURATION_H
#define SENTRY_CONFIGURATION_H

#include "sentry/sentry_options.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

using namespace godot;

namespace sentry {

class SentryConfiguration : public Node {
	GDCLASS(SentryConfiguration, Node);

protected:
	static void _bind_methods();
	void _notification(int p_what);

	GDVIRTUAL1(_configure, Ref<SentryOptions>);

	void _call_configure(const Ref<SentryOptions> &p_options);

public:
	SentryConfiguration() {}
};

} // namespace sentry

#endif // SENTRY_CONFIGURATION_H
