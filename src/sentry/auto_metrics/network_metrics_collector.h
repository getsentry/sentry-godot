#pragma once

#include "metrics_collector.h"

#include <godot_cpp/core/object_id.hpp>

namespace sentry {

class AutoMetricsAttributes;

// Collects remote peer counts, cached ENet statistics, and ENet traffic rates.
class NetworkMetricsCollector final : public MetricsCollector {
public:
	enum class Topology {
		CLIENT,
		SERVER,
		MESH,
	};

	struct Snapshot {
		godot::ObjectID host_id;
		double bytes_received = 0.0;
		double bytes_sent = 0.0;
		double packets_received = 0.0;
		double packets_sent = 0.0;
		// Negative means unavailable.
		double packet_loss = -1.0;
		double total_ping_msec = 0.0;
		double max_ping_msec = 0.0;
		uint32_t num_peers = 0;
		uint32_t num_ping_readings = 0;
		Topology topology = Topology::CLIENT;

		void add_ping(double p_ping_msec);
	};

	using SnapshotReader = bool (*)(Snapshot &);

private:
	AutoMetricsAttributes &_attributes;
	uint64_t _deadline = 0;
	godot::ObjectID _last_traffic_host_id;
	uint64_t _last_traffic_usec = 0;
	SnapshotReader _snapshot_reader;

	static bool _read_network_stats(Snapshot &p_snapshot);

protected:
	void _process(uint64_t p_now_usec) override;

public:
	explicit NetworkMetricsCollector(AutoMetricsAttributes &p_attributes, SnapshotReader p_snapshot_reader = &_read_network_stats) :
			_attributes(p_attributes), _snapshot_reader(p_snapshot_reader) {}

	void reset() override;
};

} // namespace sentry
