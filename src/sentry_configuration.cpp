#include "sentry_configuration.h"

#include "sentry_sdk.h"

namespace sentry {

void SentryConfiguration::_call_configure(const Ref<SentryOptions> &p_options) {
	ERR_FAIL_COND(p_options.is_null());
	GDVIRTUAL_CALL(_configure, p_options);
	ERR_FAIL_NULL(SentrySDK::get_singleton());
	SentrySDK::get_singleton()->notify_options_configured();
}

void SentryConfiguration::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		_call_configure(SentryOptions::get_singleton());
	}
}

void SentryConfiguration::_bind_methods() {
	GDVIRTUAL_BIND(_configure, "options");
}

};