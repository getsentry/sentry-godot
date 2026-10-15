#pragma once

#include "metrics_collector.h"
#include "sentry/platform/platform.h"

namespace sentry {

class AutoMetricsAttributes;

// Collects process resident memory and Godot object and scene-tree node counts.
class MemoryMetricsCollector final : public MetricsCollector {
public:
	using ResidentMemoryReader = bool (*)(uint64_t &);

private:
	AutoMetricsAttributes &_attributes;
	uint64_t _deadline = 0;

	// Reads the resident memory used by the current process, in bytes.
	ResidentMemoryReader _read_used_memory;

protected:
	void _process(uint64_t p_now_usec) override;

public:
	explicit MemoryMetricsCollector(AutoMetricsAttributes &p_attributes,
			ResidentMemoryReader p_mem_reader = &sentry::platform::get_process_resident_memory) :
			_attributes(p_attributes), _read_used_memory(p_mem_reader) {}

	void reset() override;
};

} // namespace sentry
