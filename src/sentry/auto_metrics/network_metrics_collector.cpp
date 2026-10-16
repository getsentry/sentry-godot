#include "network_metrics_collector.h"

#include "auto_metrics_attributes.h"
#include "sentry/logging/print.h"
#include "sentry/sentry_sdk.h"
#include "sentry/sentry_unit.h"

#include <godot_cpp/classes/e_net_multiplayer_peer.hpp>
#include <godot_cpp/classes/e_net_packet_peer.hpp>
#include <godot_cpp/classes/multiplayer_api.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/math.hpp>

namespace sentry {

void NetworkMetricsCollector::Snapshot::add_ping(double p_ping_msec) {
	++num_peers;
	total_ping_msec += p_ping_msec;
	max_ping_msec = MAX(max_ping_msec, p_ping_msec);
}

bool NetworkMetricsCollector::_read_enet_statistics(Snapshot &p_snapshot) {
	SceneTree *tree = SceneTree::get_singleton();
	if (tree == nullptr) {
		SENTRY_PRINT_ONCE(sentry::LEVEL_WARNING, "SceneTree is not available. Network metrics will not be collected.");
		return false;
	}
	Ref<MultiplayerAPI> multiplayer = tree->get_multiplayer();
	if (multiplayer.is_null()) {
		return false;
	}
	Ref<ENetMultiplayerPeer> enet = multiplayer->get_multiplayer_peer();
	if (enet.is_null() || enet->get_connection_status() != MultiplayerPeer::CONNECTION_CONNECTED) {
		return false;
	}

	if (!enet->is_server_relay_supported()) {
		p_snapshot.topology = Topology::MESH;
	} else if (multiplayer->is_server()) {
		p_snapshot.topology = Topology::SERVER;
	} else {
		p_snapshot.topology = Topology::CLIENT;
	}

	for (int32_t peer_id : multiplayer->get_peers()) {
		if (p_snapshot.topology == Topology::CLIENT && peer_id != MultiplayerPeer::TARGET_PEER_SERVER) {
			// If client, we are only interested in server.
			continue;
		}

		Ref<ENetPacketPeer> peer = enet->get_peer(peer_id);
		if (peer.is_valid() && peer->get_state() == ENetPacketPeer::STATE_CONNECTED) {
			p_snapshot.add_ping(peer->get_statistic(ENetPacketPeer::PEER_ROUND_TRIP_TIME));
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
	if (!_snapshot_reader(snapshot) || snapshot.num_peers == 0) {
		return;
	}

	const Dictionary &attributes = _attributes.get_current();
	const String &unit = SentryUnit::get_singleton()->get_millisecond();
	const double mean_ping_msec = snapshot.total_ping_msec / snapshot.num_peers;
	switch (snapshot.topology) {
		case Topology::CLIENT: {
			SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.ping", mean_ping_msec, unit, attributes);
		} break;
		case Topology::SERVER:
		case Topology::MESH: {
			SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.ping_peer_avg", mean_ping_msec, unit, attributes);
			SentrySDK::get_singleton()->get_metrics()->gauge("game.perf.net.ping_peer_max", snapshot.max_ping_msec, unit, attributes);
		} break;
	}
}

void NetworkMetricsCollector::reset() {
	_deadline = 0;
}

} // namespace sentry
