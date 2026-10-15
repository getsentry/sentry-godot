#include "sentry_auto_metrics.h"

#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/time.hpp>

namespace sentry {

void SentryAutoMetrics::_process_collectors() {
	uint64_t now = Time::get_singleton()->get_ticks_usec();
	frame_metrics_collector.process(now);
	rendering_metrics_collector.process(now);
	memory_metrics_collector.process(now);
}

void SentryAutoMetrics::_start_collection_if_app_is_active() {
	if (_app_paused || !_app_focused || is_processing_internal() || is_queued_for_deletion()) {
		return;
	}
	const Ref<SentryAutoMetricsOptions> &opts = SENTRY_OPTIONS()->get_auto_metrics();
	frame_metrics_collector.set_enabled(opts->get_enable_frame_metrics());
	frame_metrics_collector.reset();
	rendering_metrics_collector.set_enabled(opts->get_enable_rendering_metrics());
	rendering_metrics_collector.reset();
	memory_metrics_collector.set_enabled(opts->get_enable_memory_metrics());
	memory_metrics_collector.reset();
	set_process_internal(true);
}

void SentryAutoMetrics::_stop_collection() {
	if (is_processing_internal()) {
		set_process_internal(false);
		frame_metrics_collector.reset();
		rendering_metrics_collector.reset();
		memory_metrics_collector.reset();
	}
}

void SentryAutoMetrics::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			_process_collectors();
		} break;
		case NOTIFICATION_ENTER_TREE: {
			_attributes.setup();
			_start_collection_if_app_is_active();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_stop_collection();
			_attributes.teardown();
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

SentryAutoMetrics::SentryAutoMetrics() :
		frame_metrics_collector(_attributes), rendering_metrics_collector(_attributes), memory_metrics_collector(_attributes) {
	set_process_internal(false);
	set_process_mode(PROCESS_MODE_ALWAYS);
}

} //namespace sentry
