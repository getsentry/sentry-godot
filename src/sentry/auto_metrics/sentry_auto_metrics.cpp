#include "sentry_auto_metrics.h"

#include "sentry/sentry_sdk.h"
#include <godot_cpp/classes/time.hpp>

namespace sentry {

void SentryAutoMetrics::_process_collectors() {
	uint64_t now = Time::get_singleton()->get_ticks_usec();
	frame_metrics_collector.process(now);
}

void SentryAutoMetrics::start_collection() {
	frame_metrics_collector.set_enabled(SENTRY_OPTIONS()->get_auto_metrics()->get_enable_frame_metrics());
	frame_metrics_collector.reset();
}

void SentryAutoMetrics::stop_collection() {
	frame_metrics_collector.set_enabled(false);
}

void SentryAutoMetrics::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			_process_collectors();
		} break;
	}
}

SentryAutoMetrics::SentryAutoMetrics() {
	set_process_internal(true);
	set_process_mode(PROCESS_MODE_ALWAYS);
}

} //namespace sentry
