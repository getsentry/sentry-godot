// Tests the frame metrics collector directly.
// SDK integration is tested in project/test/isolated/test_auto_metrics_lifecycle.gd.

#if defined(TESTS_ENABLED) && defined(SDK_NATIVE)

#include "cpp_test_helpers.h"
#include "sentry/auto_metrics/frame_metrics_collector.h"
#include "sentry/sentry_metric.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <vector>

using namespace godot;
using namespace sentry;

namespace {

std::vector<Ref<SentryMetric>> captured_metrics;

Ref<SentryMetric> _capture_metric(const Ref<SentryMetric> &p_metric) {
	captured_metrics.push_back(p_metric);
	return {};
}

void _configure_sdk(const Ref<SentryOptions> &p_options, double p_interval_sec) {
	p_options->set_dsn("https://public@127.0.0.1/1");
	p_options->set_debug_enabled(false);
	p_options->set_shutdown_timeout_ms(0);
	p_options->get_godot_logger()->set_enabled(false);
	p_options->set_before_send_metric(callable_mp_static(&_capture_metric));
	p_options->get_auto_metrics()->set_enable_frame_metrics(false); // collector instantiated manually
	p_options->get_auto_metrics()->set_enable_rendering_metrics(false);
	p_options->get_auto_metrics()->set_enable_memory_metrics(false);
	p_options->get_auto_metrics()->set_enable_network_metrics(false);
	p_options->get_auto_metrics()->set_frame_metrics_interval_sec(p_interval_sec);
}

class InitFixture {
public:
	explicit InitFixture(double p_interval_sec) {
		captured_metrics.clear();
		SentrySDK::get_singleton()->init(callable_mp_static(&_configure_sdk).bind(p_interval_sec));
	}

	~InitFixture() {
		SentrySDK::get_singleton()->close();
		captured_metrics.clear();
	}
};

} // namespace

TEST_SUITE("Frame metrics collector") {
	TEST_CASE("Emits nothing when disabled and starts a fresh window when enabled") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		FrameMetricsCollector collector;
		collector.process(1'000'000);
		collector.process(2'000'000);
		CHECK(captured_metrics.empty());
		collector.set_enabled(true);
		collector.process(3'000'000); // establishes a baseline at 3 seconds
		CHECK(captured_metrics.empty());
		collector.process(4'000'000); // 1s passed since baseline
		CHECK(captured_metrics.size() == 2);
	}

	TEST_CASE("Emits frame time and FPS with the expected names, types, values, and units") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		FrameMetricsCollector collector;
		collector.set_enabled(true);
		collector.process(1'000'000); // establishes a baseline
		collector.process(2'500'000); // 1500 ms passed
		const double fps = Performance::get_singleton()->get_monitor(Performance::TIME_FPS);
		REQUIRED_CHECK(captured_metrics.size() == 2);
		CHECK(captured_metrics[0]->get_name() == "game.perf.frame_time");
		CHECK(captured_metrics[0]->get_type() == SentryMetric::METRIC_DISTRIBUTION);
		CHECK(captured_metrics[0]->get_unit() == "millisecond");
		CHECK(captured_metrics[0]->get_value() == 1500.0);
		CHECK(captured_metrics[1]->get_name() == "game.perf.fps");
		CHECK(captured_metrics[1]->get_type() == SentryMetric::METRIC_GAUGE);
		CHECK(captured_metrics[1]->get_unit().is_empty());
		CHECK(captured_metrics[1]->get_value() == fps);
	}

	TEST_CASE("Emits metrics only when the configured interval has elapsed") {
		InitFixture fixture(1.25);
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		FrameMetricsCollector collector;
		collector.set_enabled(true);
		collector.process(1'000'000); // baseline
		collector.process(2'249'999); // 1.24999s passed
		CHECK(captured_metrics.empty());
		collector.process(2'250'000); // 1.25s passed since baseline
		CHECK(captured_metrics.size() == 2);
		collector.process(3'499'999); // 1.24999s since last collection
		CHECK(captured_metrics.size() == 2);
		collector.process(3'500'000); // 1.25s since last collection
		CHECK(captured_metrics.size() == 4);
	}

	TEST_CASE("A long frame stall emits only once and schedules the next window from the current time point") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		FrameMetricsCollector collector;
		collector.set_enabled(true);
		collector.process(1'000'000); // baseline
		collector.process(121'000'000); // stall
		REQUIRED_CHECK(captured_metrics.size() == 2);
		CHECK(captured_metrics[0]->get_value() == 120'000.0);
		collector.process(121'999'999); // 0.99s since last collection
		CHECK(captured_metrics.size() == 2);
		collector.process(122'000'000); // 1.0s since last collection
		CHECK(captured_metrics.size() == 4);
	}

	TEST_CASE("Reset discards the partial window and starts a fresh baseline") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		FrameMetricsCollector collector;
		collector.set_enabled(true);
		collector.process(1'000'000);
		collector.process(1'250'000);
		collector.reset();
		collector.process(10'000'000); // new baseline after reset
		CHECK(captured_metrics.empty());
		collector.process(10'500'000); // 0.5s since baseline
		CHECK(captured_metrics.empty());
		collector.process(11'000'000); // 1.0s since baseline
		REQUIRED_CHECK(captured_metrics.size() == 2);
		CHECK(captured_metrics[0]->get_value() == 500.0);
	}
}

#endif // TESTS_ENABLED && SDK_NATIVE
