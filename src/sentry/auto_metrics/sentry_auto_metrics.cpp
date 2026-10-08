#include "sentry_auto_metrics.h"

#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/time.hpp>

namespace sentry {

void SentryAutoMetrics::_process_collectors() {
	uint64_t now = Time::get_singleton()->get_ticks_usec();
	frame_metrics_collector.process(now);
}

void SentryAutoMetrics::_start_collection_if_app_is_active() {
	if (_app_paused || !_app_focused || is_processing_internal() || is_queued_for_deletion()) {
		return;
	}
	frame_metrics_collector.set_enabled(SENTRY_OPTIONS()->get_auto_metrics()->get_enable_frame_metrics());
	frame_metrics_collector.reset();
	set_process_internal(true);
}

void SentryAutoMetrics::_stop_collection() {
	if (is_processing_internal()) {
		set_process_internal(false);
		frame_metrics_collector.reset();
	}
}

void SentryAutoMetrics::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			_process_collectors();
		} break;
		case NOTIFICATION_ENTER_TREE: {
			_start_collection_if_app_is_active();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_stop_collection();
		} break;
		case NOTIFICATION_APPLICATION_PAUSED: {
			_app_paused = true;
			_stop_collection();
		} break;
		case NOTIFICATION_APPLICATION_RESUMED: {
			_app_paused = false;
			_start_collection_if_app_is_active();
		} break;
		case NOTIFICATION_APPLICATION_FOCUS_OUT: {
			_app_focused = false;
			_stop_collection();
		} break;
		case NOTIFICATION_APPLICATION_FOCUS_IN: {
			_app_focused = true;
			_start_collection_if_app_is_active();
		} break;
	}
}

SentryAutoMetrics::SentryAutoMetrics() {
	set_process_internal(false);
	set_process_mode(PROCESS_MODE_ALWAYS);
}

} //namespace sentry
