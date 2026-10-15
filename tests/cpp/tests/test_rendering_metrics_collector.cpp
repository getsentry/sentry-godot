// Tests the rendering metrics collector directly.
// SDK integration is tested in project/test/isolated/test_auto_metrics_lifecycle.gd.

#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/auto_metrics/auto_metrics_attributes.h"
#include "sentry/auto_metrics/rendering_metrics_collector.h"
#include "sentry/sentry_metric.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <vector>

using namespace godot;
using namespace sentry;

namespace {

std::vector<Ref<SentryMetric>> captured_metrics;
RenderingMetricsCollector::Snapshot snapshot;
int snapshot_reads = 0;
bool snapshot_available = true;

bool read_snapshot(RenderingMetricsCollector::Snapshot &p_snapshot) {
	++snapshot_reads;
	p_snapshot = snapshot;
	return snapshot_available;
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
	p_options->get_auto_metrics()->set_rendering_metrics_interval_sec(p_interval_sec);
}

class InitFixture {
public:
	AutoMetricsAttributes attributes;

	explicit InitFixture(double p_interval_sec) {
		captured_metrics.clear();
		snapshot = {};
		snapshot_reads = 0;
		snapshot_available = true;
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

TEST_SUITE("Rendering metrics collector") {
	TEST_CASE("Emits nothing when disabled and starts a fresh window when enabled") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		RenderingMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.process(1'000'000);
		collector.process(2'000'000);
		CHECK(captured_metrics.empty());
		collector.set_enabled(true);
		collector.process(3'000'000); // establishes a baseline at 3 seconds
		CHECK(captured_metrics.empty());
		collector.process(4'000'000); // 1s passed since baseline
		CHECK(captured_metrics.size() == 5);
	}

	TEST_CASE("Emits rendering metrics with the expected names, types, values, and units") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());

		snapshot.draw_calls = 12.0;
		snapshot.primitives = 345.0;
		snapshot.video_mem_used = 8'000'000'000.0;
		snapshot.texture_mem_used = 6'000'000'000.0;
		snapshot.buffer_mem_used = 1'000'000'000.0;

		const bool is_headless = DisplayServer::get_singleton()->get_name() == "headless";
		RenderingMetricsCollector collector = is_headless
				? RenderingMetricsCollector(fixture.attributes, &read_snapshot)
				: RenderingMetricsCollector(fixture.attributes);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);
		const struct {
			const char *name;
			double value;
			const char *unit;
			Performance::Monitor monitor;
		} expected[] = {
			{ "game.perf.render.draw_calls", 12.0, "", Performance::RENDER_TOTAL_DRAW_CALLS_IN_FRAME },
			{ "game.perf.render.primitives", 345.0, "", Performance::RENDER_TOTAL_PRIMITIVES_IN_FRAME },
			{ "game.perf.render.video_mem_used", 8'000'000'000.0, "byte", Performance::RENDER_VIDEO_MEM_USED },
			{ "game.perf.render.texture_mem_used", 6'000'000'000.0, "byte", Performance::RENDER_TEXTURE_MEM_USED },
			{ "game.perf.render.buffer_mem_used", 1'000'000'000.0, "byte", Performance::RENDER_BUFFER_MEM_USED },
		};
		REQUIRED_CHECK(captured_metrics.size() == 5);
		for (const auto &reading : expected) {
			CAPTURE(reading.name);
			Ref<SentryMetric> metric = find_metric(reading.name);
			REQUIRED_CHECK(metric.is_valid());
			CHECK(metric->get_type() == SentryMetric::METRIC_GAUGE);
			const double expected_value = is_headless
					? reading.value
					: Performance::get_singleton()->get_monitor(reading.monitor);
			CHECK(metric->get_value() == expected_value);
			CHECK(metric->get_unit() == reading.unit);
		}
	}

	TEST_CASE("Attaches the shared attributes to every rendering metric") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		RenderingMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(2'000'000);
		REQUIRED_CHECK(captured_metrics.size() == 5);
		const Dictionary &attributes = fixture.attributes.get_current();
		REQUIRED_CHECK(attributes.has("cpu.cores"));
		for (const auto &metric : captured_metrics) {
			CAPTURE(metric->get_name());
			CHECK(metric->get_attribute("cpu.cores") == attributes["cpu.cores"]);
		}
	}

	TEST_CASE("Emits metrics only when the configured interval has elapsed") {
		constexpr double interval = 1.25;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		RenderingMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000); // baseline
		collector.process(2'249'999); // just before the reporting deadline
		CHECK(captured_metrics.empty());
		CHECK(snapshot_reads == 0);
		collector.process(2'250'000); // 1.25s passed since baseline
		CHECK(captured_metrics.size() == 5);
		CHECK(snapshot_reads == 1);
		collector.process(3'499'999); // just before the next reporting deadline
		CHECK(captured_metrics.size() == 5);
		CHECK(snapshot_reads == 1);
		collector.process(3'500'000); // 1.25s since last collection
		CHECK(captured_metrics.size() == 10);
		CHECK(snapshot_reads == 2);
	}

	TEST_CASE("A long stall emits once and starts a new reporting window") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		RenderingMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(121'000'000);
		CHECK(snapshot_reads == 1);
		CHECK(captured_metrics.size() == 5);
		collector.process(121'999'999);
		CHECK(snapshot_reads == 1);
		collector.process(122'000'000);
		CHECK(snapshot_reads == 2);
		CHECK(captured_metrics.size() == 10);
	}

	TEST_CASE("Reset starts a new reporting interval") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		RenderingMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(1'500'000);
		collector.reset();
		collector.process(10'000'000);
		collector.process(10'999'999);
		CHECK(snapshot_reads == 0);
		CHECK(captured_metrics.empty());
		collector.process(11'000'000);
		CHECK(snapshot_reads == 1);
		CHECK(captured_metrics.size() == 5);
	}

	TEST_CASE("Unavailable snapshots emit nothing and retry at the reporting interval") {
		// Godot may disable some metrics via compile-time flags in the future.
		// Test that we handle that gracefully.
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		RenderingMetricsCollector collector(fixture.attributes, &read_snapshot);
		collector.set_enabled(true);
		snapshot_available = false;
		collector.process(1'000'000);
		collector.process(2'000'000);
		CHECK(snapshot_reads == 1);
		CHECK(captured_metrics.empty());
		collector.process(2'500'000);
		CHECK(snapshot_reads == 1);
		collector.process(3'000'000);
		CHECK(snapshot_reads == 2);
		CHECK(captured_metrics.empty());
	}
}

#endif // TESTS_ENABLED
