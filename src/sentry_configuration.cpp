#include "sentry_configuration.h"

void SentryConfiguration::_call_initialize(const Ref<SentryOptions> &p_options) {
	GDVIRTUAL_CALL(_initialize, p_options);
}

void SentryConfiguration::_bind_methods() {
	GDVIRTUAL_BIND(_initialize, "options");
}
