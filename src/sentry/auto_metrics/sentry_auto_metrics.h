#pragma once

#include "frame_metrics_collector.h"

#include <godot_cpp/classes/node.hpp>

using namespace godot;

namespace sentry {

// Scene tree node coordinating automatic metrics collection.
class SentryAutoMetrics : public Node {
	GDCLASS(SentryAutoMetrics, Node);

private:
	FrameMetricsCollector frame_metrics_collector;
	bool _app_focused = true;
	bool _app_paused = false;

	void _start_collection_if_app_is_active();
	void _stop_collection();

	void _process_collectors();

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	SentryAutoMetrics();
};

} //namespace sentry
