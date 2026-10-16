#pragma once

#include "metrics_collector.h"

namespace sentry {

class AutoMetricsAttributes;

// Collects cached ENet round-trip times.
class NetworkMetricsCollector final : public MetricsCollector {
public:
	enum class Topology {
		CLIENT,
		SERVER,
		MESH,
	};

	struct Snapshot {
		double total_ping_msec = 0.0;
		double max_ping_msec = 0.0;
		uint32_t num_peers = 0;
		Topology topology = Topology::CLIENT;

		void add_ping(double p_ping_msec);
	};

	using SnapshotReader = bool (*)(Snapshot &);

private:
	AutoMetricsAttributes &_attributes;
	uint64_t _deadline = 0;
	SnapshotReader _snapshot_reader;

	static bool _read_enet_statistics(Snapshot &p_snapshot);

protected:
	void _process(uint64_t p_now_usec) override;

public:
	explicit NetworkMetricsCollector(AutoMetricsAttributes &p_attributes, SnapshotReader p_snapshot_reader = &_read_enet_statistics) :
			_attributes(p_attributes), _snapshot_reader(p_snapshot_reader) {}

	void reset() override;
};

} // namespace sentry
