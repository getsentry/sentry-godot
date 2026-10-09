#include "frame_metrics_collector.h"

#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/performance.hpp>

#include <random>

namespace sentry {

void FrameMetricsCollector::_process(uint64_t p_now_usec) {
	const bool baseline = _deadline == 0;
	if (unlikely(baseline)) {
		_deadline = p_now_usec + SENTRY_OPTIONS()->get_auto_metrics()->get_normalized_frame_metrics_interval_usec();
	} else {
		++_num_frames;

		// See https://en.wikipedia.org/wiki/Reservoir_sampling
		std::uniform_int_distribution<uint64_t> dist{ 1, _num_frames };
		if (dist(_rng) == 1) {
			_sampled_frametime = (p_now_usec - _last_usec) * 0.001;
		}

		if (p_now_usec >= _deadline) {
			_num_frames = 0;
			_deadline = p_now_usec + SENTRY_OPTIONS()->get_auto_metrics()->get_normalized_frame_metrics_interval_usec();
			SentrySDK::get_singleton()->get_metrics()->distribution(
					"game.perf.frame_time", _sampled_frametime, "millisecond");
			SentrySDK::get_singleton()->get_metrics()->gauge(
					"game.perf.fps",
					Performance::get_singleton()->get_monitor(Performance::Monitor::TIME_FPS));
		}
	}
	_last_usec = p_now_usec;
}

void FrameMetricsCollector::reset() {
	_num_frames = 0;
	_last_usec = 0;
	_sampled_frametime = 0.0;
	_deadline = 0;
}

} // namespace sentry
