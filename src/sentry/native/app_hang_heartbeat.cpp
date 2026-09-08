#include "app_hang_heartbeat.h"

#include "sentry.h"

namespace sentry::native {

void AppHangHeartbeat::_process(double p_delta) {
	sentry_app_hang_heartbeat();
}

AppHangHeartbeat::AppHangHeartbeat() {
	set_process(true);
}

AppHangHeartbeat::~AppHangHeartbeat() {
}

} //namespace sentry::native
