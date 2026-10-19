#include "network_metrics_collector.h"

#include "auto_metrics_attributes.h"
#include "sentry/logging/print.h"
#include "sentry/sentry_sdk.h"
#include "sentry/sentry_unit.h"

#include <godot_cpp/classes/e_net_connection.hpp>
#include <godot_cpp/classes/e_net_multiplayer_peer.hpp>
#include <godot_cpp/classes/e_net_packet_peer.hpp>
#include <godot_cpp/classes/multiplayer_api.hpp>
#include <godot_cpp/classes/offline_multiplayer_peer.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/math.hpp>

namespace sentry {

namespace {

inline void _read_enet_peer_ping(const Ref<ENetPacketPeer> &p_peer, NetworkMetricsCollector::Snapshot &p_snapshot) {
	if (p_peer.is_valid() && p_peer->get_state() == ENetPacketPeer::STATE_CONNECTED) {
		p_snapshot.add_ping(p_peer->get_statistic(ENetPacketPeer::PEER_ROUND_TRIP_TIME));
	}
}

} // namespace

void NetworkMetricsCollector::Snapshot::add_ping(double p_ping_msec) {
	++num_ping_readings;
	total_ping_msec += p_ping_msec;
	max_ping_msec = MAX(max_ping_msec, p_ping_msec);
}

bool NetworkMetricsCollector::_read_network_stats(Snapshot &p_snapshot) {
	SceneTree *tree = SceneTree::get_singleton();
	if (unlikely(tree == nullptr)) {
		SENTRY_PRINT_ONCE(sentry::LEVEL_WARNING, "SceneTree is not available. Network metrics will not be collected.");
		return false;
	}
	Ref<MultiplayerAPI> multiplayer = tree->get_multiplayer();
	if (multiplayer.is_null()) {
		return false;
	}
	Ref<MultiplayerPeer> multiplayer_peer = multiplayer->get_multiplayer_peer();
	if (multiplayer_peer.is_null() ||
			Object::cast_to<OfflineMultiplayerPeer>(multiplayer_peer.ptr()) ||
			multiplayer_peer->get_connection_status() != MultiplayerPeer::CONNECTION_CONNECTED) {
		return false;
	}

	if (!multiplayer_peer->is_server_relay_supported()) {
		p_snapshot.topology = Topology::MESH;
	} else if (multiplayer->is_server()) {
		p_snapshot.topology = Topology::SERVER;
	} else {
		p_snapshot.topology = Topology::CLIENT;
	}

	if (p_snapshot.topology != Topology::CLIENT) {
		p_snapshot.num_peers = multiplayer->get_peers().size();
	}

	Ref<ENetMultiplayerPeer> enet = multiplayer_peer;
	if (enet.is_valid()) {
		if (p_snapshot.topology != Topology::MESH) {
			// Client-server
			Ref<ENetConnection> host = enet->get_host();
			if (host.is_valid()) {
				p_snapshot.host_id = ObjectID(host->get_instance_id());
				p_snapshot.bytes_received = host->pop_statistic(ENetConnection::HOST_TOTAL_RECEIVED_DATA);
				p_snapshot.bytes_sent = host->pop_statistic(ENetConnection::HOST_TOTAL_SENT_DATA);
				p_snapshot.packets_received = host->pop_statistic(ENetConnection::HOST_TOTAL_RECEIVED_PACKETS);
				p_snapshot.packets_sent = host->pop_statistic(ENetConnection::HOST_TOTAL_SENT_PACKETS);
			}
		}

		if (p_snapshot.topology == Topology::CLIENT) {
			_read_enet_peer_ping(enet->get_peer(MultiplayerPeer::TARGET_PEER_SERVER), p_snapshot);
		} else {
			for (int32_t peer_id : multiplayer->get_peers()) {
				_read_enet_peer_ping(enet->get_peer(peer_id), p_snapshot);
			}
		}
	}

	return true;
}

void NetworkMetricsCollector::_process(uint64_t p_now_usec) {
	if (unlikely(_deadline == 0)) {
		_deadline = p_now_usec + SENTRY_OPTIONS()->get_auto_metrics()->get_normalized_network_metrics_interval_usec();
		return;
	}
	if (likely(p_now_usec < _deadline)) {
		return;
	}
	_deadline = p_now_usec + SENTRY_OPTIONS()->get_auto_metrics()->get_normalized_network_metrics_interval_usec();

	Snapshot snapshot;
	if (!_snapshot_reader(snapshot)) {
		_last_traffic_host_id = ObjectID();
		return;
	}

	const Dictionary &attributes = _attributes.get_current();

	// Num peers
	if (snapshot.topology != Topology::CLIENT) {
		SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.num_peers",
				snapshot.num_peers, String(), attributes);
	}

	// Ping
	if (snapshot.num_ping_readings > 0) {
		const String &unit = SentryUnit::get_singleton()->get_millisecond();
		const double mean_ping_msec = snapshot.total_ping_msec / snapshot.num_ping_readings;
		switch (snapshot.topology) {
			case Topology::CLIENT: {
				SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.ping",
						mean_ping_msec, unit, attributes);
			} break;
			case Topology::SERVER:
			case Topology::MESH: {
				SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.ping_peer_avg",
						mean_ping_msec, unit, attributes);
				SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.ping_peer_max",
						snapshot.max_ping_msec, unit, attributes);
			} break;
		}
	}

	// Traffic
	if (snapshot.topology != Topology::MESH) {
		// Host instance ID is used to identify the baseline.
		// Same host: divide the bytes read by elapsed time to produce rates.
		// New host or no previous baseline: discard the counters and start a new baseline.
		// No valid host: clear the baseline until a valid host appears.
		// Otherwise, we’d divide the new connection’s accumulated traffic by the old connection’s elapsed time.
		if (snapshot.host_id.is_valid() && snapshot.host_id == _last_traffic_host_id) {
			const double elapsed_sec = (p_now_usec - _last_traffic_usec) * 0.000001;
			const String &unit = SentryUnit::get_singleton()->get_byte();
			SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.in_rate",
					Math::round(snapshot.bytes_received / elapsed_sec), unit, attributes);
			SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.out_rate",
					Math::round(snapshot.bytes_sent / elapsed_sec), unit, attributes);
			SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.in_packets",
					snapshot.packets_received / elapsed_sec, String(), attributes);
			SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.out_packets",
					snapshot.packets_sent / elapsed_sec, String(), attributes);
		}
		_last_traffic_host_id = snapshot.host_id;
		_last_traffic_usec = p_now_usec;
	}
}

void NetworkMetricsCollector::reset() {
	_deadline = 0;
	_last_traffic_host_id = ObjectID();
	_last_traffic_usec = 0;
}

} // namespace sentry
