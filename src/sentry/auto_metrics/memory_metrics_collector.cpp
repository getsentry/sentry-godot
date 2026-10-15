#include "memory_metrics_collector.h"

#include "auto_metrics_attributes.h"
#include "sentry/logging/print.h"
#include "sentry/sentry_sdk.h"
#include "sentry/sentry_unit.h"

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

namespace sentry {

void MemoryMetricsCollector::_process(uint64_t p_now_usec) {
	if (unlikely(_deadline == 0)) {
		_deadline = p_now_usec + SENTRY_OPTIONS()->get_auto_metrics()->get_normalized_memory_metrics_interval_usec();
		return;
	}
	if (likely(p_now_usec < _deadline)) {
		return;
	}

	_deadline = p_now_usec + SENTRY_OPTIONS()->get_auto_metrics()->get_normalized_memory_metrics_interval_usec();

	const Dictionary &attributes = _attributes.get_current();
	uint64_t used_bytes = 0;
	if (_read_used_memory(used_bytes) && used_bytes > 0) {
		SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.used_memory",
				static_cast<double>(used_bytes), SentryUnit::get_singleton()->get_byte(), attributes);
	}

	Performance *performance = Performance::get_singleton();
	if (performance == nullptr) {
		SENTRY_PRINT_ONCE(sentry::LEVEL_WARNING, "Performance singleton is not available. Object and node counts will not be collected.");
		return;
	}

	double object_count = performance->get_monitor(Performance::OBJECT_COUNT);
	SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.object_count",
			object_count, String(), attributes);

	if (SceneTree::get_singleton() != nullptr) {
		double node_count = performance->get_monitor(Performance::OBJECT_NODE_COUNT);
		SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.node_count",
				node_count, String(), attributes);
	}
}

void MemoryMetricsCollector::reset() {
	_deadline = 0;
}

} // namespace sentry
