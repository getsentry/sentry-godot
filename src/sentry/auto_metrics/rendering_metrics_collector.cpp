#include "rendering_metrics_collector.h"

#include "auto_metrics_attributes.h"
#include "sentry/sentry_sdk.h"
#include "sentry/sentry_unit.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/core/math.hpp>

namespace sentry {

namespace {

inline bool _is_valid_reading(double p_value) {
	return Math::is_finite(p_value) && p_value >= 0.0;
}

} // namespace

bool RenderingMetricsCollector::_read_performance_monitors(Snapshot &p_snapshot) {
	if (DisplayServer::get_singleton()->get_name() == "headless") {
		return false;
	}
	Performance *performance = Performance::get_singleton();
	p_snapshot.draw_calls = performance->get_monitor(Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME);
	p_snapshot.primitives = performance->get_monitor(Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME);
	p_snapshot.video_mem_used = performance->get_monitor(Performance::RENDER_VIDEO_MEM_USED);
	p_snapshot.texture_mem_used = performance->get_monitor(Performance::RENDER_TEXTURE_MEM_USED);
	p_snapshot.buffer_mem_used = performance->get_monitor(Performance::RENDER_BUFFER_MEM_USED);
	return true;
}

void RenderingMetricsCollector::_process(uint64_t p_now_usec) {
	if (unlikely(_deadline == 0)) {
		_deadline = p_now_usec + SENTRY_OPTIONS()->get_auto_metrics()->get_normalized_rendering_metrics_interval_usec();
		return;
	}
	if (likely(p_now_usec < _deadline)) {
		return;
	}

	_deadline = p_now_usec + SENTRY_OPTIONS()->get_auto_metrics()->get_normalized_rendering_metrics_interval_usec();

	Snapshot snapshot;
	if (!_snapshot_reader(snapshot)) {
		return;
	}

	const Dictionary &attributes = _attributes.get_current();
	const String &byte_unit = SentryUnit::get_singleton()->get_byte();
	if (_is_valid_reading(snapshot.draw_calls)) {
		SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.render.draw_calls",
				snapshot.draw_calls, String(), attributes);
	}
	if (_is_valid_reading(snapshot.primitives)) {
		SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.render.primitives",
				snapshot.primitives, String(), attributes);
	}
	if (_is_valid_reading(snapshot.video_mem_used)) {
		SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.render.video_mem_used",
				snapshot.video_mem_used, byte_unit, attributes);
	}
	if (_is_valid_reading(snapshot.texture_mem_used)) {
		SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.render.texture_mem_used",
				snapshot.texture_mem_used, byte_unit, attributes);
	}
	if (_is_valid_reading(snapshot.buffer_mem_used)) {
		SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.render.buffer_mem_used",
				snapshot.buffer_mem_used, byte_unit, attributes);
	}
}

void RenderingMetricsCollector::reset() {
	_deadline = 0;
}

} // namespace sentry
