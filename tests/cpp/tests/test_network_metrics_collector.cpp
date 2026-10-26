// Tests network metric collection.
// SDK lifecycle is tested in project/test/isolated/test_auto_metrics_lifecycle.gd.

#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/auto_metrics/auto_metrics_attributes.h"
#include "sentry/auto_metrics/network_metrics_collector.h"
#include "sentry/sentry_metric.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/e_net_multiplayer_peer.hpp>
#include <godot_cpp/classes/multiplayer_api.hpp>
#include <godot_cpp/classes/offline_multiplayer_peer.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/web_rtc_multiplayer_peer.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <vector>

using namespace godot;
using namespace sentry;

namespace {

std::vector<Ref<SentryMetric>> captured_metrics;
std::vector<double> peer_pings;
NetworkMetricsCollector::Snapshot snapshot;
bool snapshot_available = true;
int snapshot_reads = 0;

// Supplies net stat snapshots and counts reads.
bool read_snapshot(NetworkMetricsCollector::Snapshot &p_snapshot) {
	++snapshot_reads;
	if (!snapshot_available) {
		return false;
	}
	p_snapshot = snapshot;
	for (double ping : peer_pings) {
		p_snapshot.add_ping(ping);
	}
	return true;
}

Ref<SentryMetric> record_metric(const Ref<SentryMetric> &p_metric) {
	captured_metrics.push_back(p_metric);
	return {};
}

Ref<SentryMetric> find_metric(const String &p_name) {
	for (const auto &metric : captured_metrics) {
		if (metric->get_name() == p_name) {
			return metric;
		}
	}
	return {};
}

void configure_sdk(const Ref<SentryOptions> &p_options, double p_interval_sec) {
	p_options->set_dsn("https://public@127.0.0.1/1");
	p_options->set_debug_enabled(false);
	p_options->set_shutdown_timeout_ms(0);
	p_options->get_godot_logger()->set_enabled(false);
	p_options->set_before_send_metric(callable_mp_static(&record_metric));
	p_options->get_auto_metrics()->set_network_metrics_interval_sec(p_interval_sec);
}

class InitFixture {
public:
	AutoMetricsAttributes attributes;

	explicit InitFixture(double p_interval_sec) {
		captured_metrics.clear();
		peer_pings.clear();
		snapshot = {};
		snapshot.num_peers = 1;
		snapshot_available = true;
		snapshot_reads = 0;
		SentrySDK::get_singleton()->init(callable_mp_static(&configure_sdk).bind(p_interval_sec));
		attributes.setup();
	}

	~InitFixture() {
		attributes.teardown();
		SentrySDK::get_singleton()->close();
		captured_metrics.clear();
	}
};

} // namespace

TEST_SUITE("Network metrics collector") {
	TEST_CASE("Clients emit ping without peer counts or peer ping aggregates") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		SUBCASE("Nonzero RTT") { peer_pings = { 42.0 }; }
		SUBCASE("Zero RTT") { peer_pings = { 0.0 }; }
		snapshot.topology = NetworkMetricsCollector::Topology::CLIENT;
		snapshot.num_peers = 2;

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000); // baseline
		collector.process(2'000'000); // 1s since baseline

		Ref<SentryMetric> ping = find_metric("game.perf.net.ping");
		REQUIRED_CHECK(ping.is_valid());
		CHECK(ping->get_value() == peer_pings[0]);
		CHECK(ping->get_type() == SentryMetric::METRIC_GAUGE);
		CHECK(ping->get_unit() == "millisecond");
		CHECK(find_metric("game.perf.net.num_peers").is_null());
		CHECK(find_metric("game.perf.net.ping_peer_avg").is_null());
		CHECK(find_metric("game.perf.net.ping_peer_max").is_null());
	}

	TEST_CASE("Servers and mesh peers emit ping aggregates") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		SUBCASE("Server") { snapshot.topology = NetworkMetricsCollector::Topology::SERVER; }
		SUBCASE("Mesh") { snapshot.topology = NetworkMetricsCollector::Topology::MESH; }
		peer_pings = { 12.0, 100.0, 32.0 };
		snapshot.num_peers = 3;

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);

		Ref<SentryMetric> average = find_metric("game.perf.net.ping_peer_avg");
		Ref<SentryMetric> maximum = find_metric("game.perf.net.ping_peer_max");
		REQUIRED_CHECK(average.is_valid());
		REQUIRED_CHECK(maximum.is_valid());
		CHECK(average->get_value() == 48.0);
		CHECK(maximum->get_value() == 100.0);
		CHECK(find_metric("game.perf.net.ping").is_null());
		for (const auto &metric : { average, maximum }) {
			CHECK(metric->get_type() == SentryMetric::METRIC_GAUGE);
			CHECK(metric->get_unit() == "millisecond");
		}
	}

	TEST_CASE("Servers and mesh peers emit peer counts") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		SUBCASE("Server with connected peers") {
			snapshot.topology = NetworkMetricsCollector::Topology::SERVER;
			snapshot.num_peers = 3;
		}
		SUBCASE("Server without connected peers") {
			snapshot.topology = NetworkMetricsCollector::Topology::SERVER;
			snapshot.num_peers = 0;
		}
		SUBCASE("Mesh with connected peers") {
			snapshot.topology = NetworkMetricsCollector::Topology::MESH;
			snapshot.num_peers = 3;
		}
		SUBCASE("Mesh without connected peers") {
			snapshot.topology = NetworkMetricsCollector::Topology::MESH;
			snapshot.num_peers = 0;
		}

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);

		Ref<SentryMetric> metric = find_metric("game.perf.net.num_peers");
		REQUIRED_CHECK(metric.is_valid());
		CHECK(metric->get_value() == snapshot.num_peers);
		CHECK(metric->get_type() == SentryMetric::METRIC_GAUGE);
		CHECK(metric->get_unit().is_empty());
	}

	TEST_CASE("Clients emit ENet packet loss as a ratio gauge") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		snapshot.topology = NetworkMetricsCollector::Topology::CLIENT;
		// Packet loss in ENet is tracked as X / 65'536
		SUBCASE("Nonzero packet loss") { snapshot.packet_loss = 3'277.0; }
		SUBCASE("Zero packet loss") { snapshot.packet_loss = 0.0; }

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);

		Ref<SentryMetric> loss = find_metric("game.perf.net.packet_loss_ratio");
		REQUIRED_CHECK(loss.is_valid());
		CHECK(loss->get_value() == doctest::Approx(snapshot.packet_loss / 65'536.0));
		CHECK(loss->get_type() == SentryMetric::METRIC_GAUGE);
		CHECK(loss->get_unit() == "ratio");
	}

	TEST_CASE("Emits rounded byte rates using elapsed time between reads") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		snapshot.host_id = ObjectID(uint64_t(1));

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);
		snapshot.bytes_received = 6'001.0;
		snapshot.bytes_sent = 3'002.0;
		collector.process(4'500'000); // A delayed callback spans 2.5 seconds.

		Ref<SentryMetric> in_rate = find_metric("game.perf.net.in_rate");
		Ref<SentryMetric> out_rate = find_metric("game.perf.net.out_rate");
		REQUIRED_CHECK(in_rate.is_valid());
		REQUIRED_CHECK(out_rate.is_valid());
		CHECK(in_rate->get_value() == 2'400.0); // 6001 bytes / 2.5 seconds = 2400.4, rounded down.
		CHECK(out_rate->get_value() == 1'201.0); // 3002 bytes / 2.5 seconds = 1200.8, rounded up.
		for (const auto &metric : { in_rate, out_rate }) {
			CHECK(metric->get_type() == SentryMetric::METRIC_GAUGE);
			CHECK(metric->get_unit() == "byte");
		}
	}

	TEST_CASE("Emits fractional packet rates using elapsed time between reads") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		snapshot.host_id = ObjectID(uint64_t(1));

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);
		snapshot.packets_received = 9.0;
		snapshot.packets_sent = 15.0;
		collector.process(4'500'000); // A delayed callback spans 2.5 seconds.

		Ref<SentryMetric> in_packets = find_metric("game.perf.net.in_packets");
		Ref<SentryMetric> out_packets = find_metric("game.perf.net.out_packets");
		REQUIRED_CHECK(in_packets.is_valid());
		REQUIRED_CHECK(out_packets.is_valid());
		CHECK(in_packets->get_value() == doctest::Approx(3.6)); // 9 packets / 2.5 seconds.
		CHECK(out_packets->get_value() == 6.0); // 15 packets / 2.5 seconds.
		for (const auto &metric : { in_packets, out_packets }) {
			CHECK(metric->get_type() == SentryMetric::METRIC_GAUGE);
			CHECK(metric->get_unit().is_empty());
		}
	}

	TEST_CASE("Emits zero traffic rates when no traffic is recorded") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		snapshot.host_id = ObjectID(uint64_t(1));
		snapshot.bytes_received = 0.0;
		snapshot.bytes_sent = 0.0;
		snapshot.packets_received = 0.0;
		snapshot.packets_sent = 0.0;

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);
		collector.process(3'000'000);

		for (const char *name : { "game.perf.net.in_rate", "game.perf.net.out_rate", "game.perf.net.in_packets", "game.perf.net.out_packets" }) {
			CAPTURE(name);
			Ref<SentryMetric> metric = find_metric(name);
			REQUIRED_CHECK(metric.is_valid());
			CHECK(metric->get_value() == 0.0);
		}
	}

	TEST_CASE("Attaches shared attributes to every network metric") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		SUBCASE("Client") { snapshot.topology = NetworkMetricsCollector::Topology::CLIENT; }
		SUBCASE("Server") { snapshot.topology = NetworkMetricsCollector::Topology::SERVER; }
		peer_pings = { 42.0 };
		snapshot.host_id = ObjectID(uint64_t(1));
		snapshot.packet_loss = 3'277.0;

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);
		captured_metrics.clear();
		collector.process(3'000'000);

		const Dictionary &attributes = fixture.attributes.get_current();
		REQUIRED_CHECK(attributes.has("cpu.cores"));
		REQUIRED_CHECK(!captured_metrics.empty());
		for (const auto &metric : captured_metrics) {
			CAPTURE(metric->get_name());
			CHECK(metric->get_attribute("cpu.cores") == attributes["cpu.cores"]);
		}
	}

	TEST_CASE("Offline sessions emit no network metrics") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		Ref<MultiplayerAPI> multiplayer = SceneTree::get_singleton()->get_multiplayer();
		Ref<MultiplayerPeer> original_peer = multiplayer->get_multiplayer_peer();
		Ref<OfflineMultiplayerPeer> offline;
		offline.instantiate();
		multiplayer->set_multiplayer_peer(offline);

		NetworkMetricsCollector collector(fixture.attributes);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);

		CHECK(captured_metrics.empty());
		multiplayer->set_multiplayer_peer(original_peer);
	}

	TEST_CASE("Empty mesh sessions emit only a zero peer count") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		Ref<MultiplayerAPI> multiplayer = SceneTree::get_singleton()->get_multiplayer();
		Ref<MultiplayerPeer> original_peer = multiplayer->get_multiplayer_peer();
		Ref<MultiplayerPeer> mesh;
		SUBCASE("ENet") {
			Ref<ENetMultiplayerPeer> enet;
			enet.instantiate();
			REQUIRED_CHECK(enet->create_mesh(2) == OK);
			mesh = enet;
		}
		SUBCASE("WebRTC") {
			Ref<WebRTCMultiplayerPeer> webrtc;
			webrtc.instantiate();
			REQUIRED_CHECK(webrtc->create_mesh(2) == OK);
			mesh = webrtc;
		}
		multiplayer->set_multiplayer_peer(mesh);

		NetworkMetricsCollector collector(fixture.attributes);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);
		captured_metrics.clear();
		collector.process(3'000'000);
		multiplayer->set_multiplayer_peer(original_peer);

		Ref<SentryMetric> peers = find_metric("game.perf.net.num_peers");
		REQUIRED_CHECK(peers.is_valid());
		CHECK(peers->get_value() == 0.0);
	}
}

TEST_SUITE("Network metrics collector timing") {
	TEST_CASE("Enabling collection starts and repeats the configured reporting interval") {
		constexpr double interval = 1.25;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.process(1'000'000);
		collector.process(2'000'000);
		collector.set_enabled(true);
		collector.process(3'000'000); // baseline
		collector.process(4'249'999); // just before the reporting deadline
		CHECK(snapshot_reads == 0);
		collector.process(4'250'000);
		CHECK(snapshot_reads == 1);
		collector.process(5'499'999); // just before the next reporting deadline
		CHECK(snapshot_reads == 1);
		collector.process(5'500'000);
		CHECK(snapshot_reads == 2);
	}

	TEST_CASE("A long stall emits once and starts a new reporting window") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(121'000'000);
		CHECK(snapshot_reads == 1);
		collector.process(121'999'999);
		CHECK(snapshot_reads == 1);
		collector.process(122'000'000);
		CHECK(snapshot_reads == 2);
	}

	TEST_CASE("Reset starts a new reporting interval") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(1'500'000);
		collector.reset();
		collector.process(10'000'000); // new baseline
		collector.process(10'999'999);
		CHECK(snapshot_reads == 0);
		collector.process(11'000'000);
		CHECK(snapshot_reads == 1);
	}

	TEST_CASE("The first read establishes a baseline without emitting rates") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		snapshot.host_id = ObjectID(uint64_t(1));
		snapshot.bytes_received = 100'000.0;
		snapshot.bytes_sent = 200'000.0;
		snapshot.packets_received = 500.0;
		snapshot.packets_sent = 900.0;
		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);
		CHECK(snapshot_reads == 1);
		CHECK(captured_metrics.empty());
	}

	TEST_CASE("Reset discards the previous traffic baseline") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		snapshot.host_id = ObjectID(uint64_t(1));

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);

		collector.reset();
		collector.process(4'000'000);
		snapshot.bytes_received = 100'000.0;
		collector.process(5'000'000);
		CHECK(captured_metrics.empty());
		snapshot.bytes_received = 100.0;
		collector.process(6'000'000);
		Ref<SentryMetric> rate = find_metric("game.perf.net.in_rate");
		REQUIRED_CHECK(rate.is_valid());
		CHECK(rate->get_value() == 100.0);
	}

	TEST_CASE("A new ENet host starts a new traffic baseline") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());

		snapshot.host_id = ObjectID(uint64_t(1));
		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);

		snapshot.host_id = ObjectID(uint64_t(2));
		snapshot.bytes_received = 100'000.0;
		collector.process(3'000'000);
		CHECK(captured_metrics.empty());
		snapshot.bytes_received = 100.0;
		collector.process(4'000'000);
		Ref<SentryMetric> rate = find_metric("game.perf.net.in_rate");
		REQUIRED_CHECK(rate.is_valid());
		CHECK(rate->get_value() == 100.0);
	}

	TEST_CASE("An unavailable session invalidates the traffic baseline") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		snapshot.host_id = ObjectID(uint64_t(1));

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);

		snapshot_available = false;
		collector.process(3'000'000);

		snapshot_available = true;
		snapshot.bytes_received = 100'000.0;
		collector.process(4'000'000);
		CHECK(captured_metrics.empty());

		snapshot.bytes_received = 100.0;
		collector.process(5'000'000);

		Ref<SentryMetric> rate = find_metric("game.perf.net.in_rate");
		REQUIRED_CHECK(rate.is_valid());
		CHECK(rate->get_value() == 100.0);
	}

	TEST_CASE("A transport without counters invalidates the traffic baseline") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		snapshot.host_id = ObjectID(uint64_t(1));

		NetworkMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);

		snapshot.host_id = ObjectID();
		collector.process(3'000'000);

		snapshot.host_id = ObjectID(uint64_t(1));
		snapshot.bytes_received = 100'000.0;
		collector.process(4'000'000);
		CHECK(captured_metrics.empty());

		snapshot.bytes_received = 100.0;
		collector.process(5'000'000);

		Ref<SentryMetric> rate = find_metric("game.perf.net.in_rate");
		REQUIRED_CHECK(rate.is_valid());
		CHECK(rate->get_value() == 100.0);
	}
}

#endif // TESTS_ENABLED
