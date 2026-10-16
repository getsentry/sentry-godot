#pragma once

#include "auto_metrics_attributes.h"
#include "frame_metrics_collector.h"
#include "memory_metrics_collector.h"
#include "network_metrics_collector.h"
#include "rendering_metrics_collector.h"

#include <godot_cpp/classes/node.hpp>

using namespace godot;

namespace sentry {

// Scene tree node coordinating automatic metrics collection.
class SentryAutoMetrics : public Node {
	GDCLASS(SentryAutoMetrics, Node);

private:
	AutoMetricsAttributes _attributes;
	FrameMetricsCollector frame_metrics_collector;
	RenderingMetricsCollector rendering_metrics_collector;
	MemoryMetricsCollector memory_metrics_collector;
	NetworkMetricsCollector network_metrics_collector;

	// Assume focused and unpaused at startup; Godot cannot reliably tell us these states.
	// We only track notifications after this node enters the tree.
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
