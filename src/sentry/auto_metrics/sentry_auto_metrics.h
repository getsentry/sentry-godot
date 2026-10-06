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

	void _process_collectors();

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	void start_collection();
	void stop_collection();

	SentryAutoMetrics();
};

} //namespace sentry
