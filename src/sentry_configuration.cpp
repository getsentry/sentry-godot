#include "sentry_configuration.h"

void SentryConfiguration::_call_initialize() {
	GDVIRTUAL_CALL(_initialize);
}

void SentryConfiguration::_bind_methods() {
	GDVIRTUAL_BIND(_initialize);
}
