#pragma once

#include "metrics_collector.h"

namespace sentry {

class AutoMetricsAttributes;

// Collects Godot object and scene-tree node counts.
class MemoryMetricsCollector final : public MetricsCollector {
private:
	AutoMetricsAttributes &_attributes;
	uint64_t _deadline = 0;

protected:
	void _process(uint64_t p_now_usec) override;

public:
	explicit MemoryMetricsCollector(AutoMetricsAttributes &p_attributes) :
			_attributes(p_attributes) {}

	void reset() override;
};

} // namespace sentry
