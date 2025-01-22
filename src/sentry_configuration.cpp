#include "sentry_configuration.h"

#include "sentry_sdk.h"

void SentryConfiguration::_call_initialize(const Ref<SentryOptions> &p_options) {
	ERR_FAIL_COND(p_options.is_null());
	GDVIRTUAL_CALL(_initialize, p_options);
}

void SentryConfiguration::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		_call_initialize(SentryOptions::get_singleton());
		ERR_FAIL_NULL(SentrySDK::get_singleton());
		SentrySDK::get_singleton()->notify_options_initialized();
	}
}

void SentryConfiguration::_bind_methods() {
	GDVIRTUAL_BIND(_initialize, "options");
}
