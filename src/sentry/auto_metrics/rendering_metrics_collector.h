#pragma once

#include "metrics_collector.h"

namespace sentry {

class AutoMetricsAttributes;

// Collects rendering counters and renderer memory allocations.
class RenderingMetricsCollector final : public MetricsCollector {
public:
	struct Snapshot {
		double draw_calls = 0.0;
		double primitives = 0.0;
		double video_mem_used = 0.0;
		double texture_mem_used = 0.0;
		double buffer_mem_used = 0.0;
	};

	// Allows collection tests to supply readings without a rendering backend during headless runs.
	using SnapshotReader = bool (*)(Snapshot &);

private:
	AutoMetricsAttributes &_attributes;
	SnapshotReader _snapshot_reader;
	uint64_t _deadline = 0;

	static bool _read_performance_monitors(Snapshot &p_snapshot);

protected:
	void _process(uint64_t p_now_usec) override;

public:
	explicit RenderingMetricsCollector(AutoMetricsAttributes &p_attributes, SnapshotReader p_snapshot_reader = &_read_performance_monitors) :
			_attributes(p_attributes), _snapshot_reader(p_snapshot_reader) {}

	void reset() override;
};

} // namespace sentry
