// Tests memory metric collection; SDK lifecycle is tested in project/test/isolated/test_auto_metrics_lifecycle.gd.

#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/auto_metrics/auto_metrics_attributes.h"
#include "sentry/auto_metrics/memory_metrics_collector.h"
#include "sentry/sentry_metric.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <vector>

using namespace godot;
using namespace sentry;

namespace {

std::vector<Ref<SentryMetric>> captured_metrics;
uint64_t resident_bytes = 0;
int resident_memory_reads = 0;
bool resident_memory_available = false;

bool read_resident_memory(uint64_t &r_bytes) {
	++resident_memory_reads;
	r_bytes = resident_bytes;
	return resident_memory_available;
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
	p_options->get_auto_metrics()->set_memory_metrics_interval_sec(p_interval_sec);
}

class InitFixture {
public:
	AutoMetricsAttributes attributes;

	explicit InitFixture(double p_interval_sec) {
		captured_metrics.clear();
		resident_bytes = 0;
		resident_memory_reads = 0;
		resident_memory_available = false;
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

TEST_SUITE("Memory metrics collector") {
	TEST_CASE("Emits nothing when disabled and starts a fresh window when enabled") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		MemoryMetricsCollector collector(fixture.attributes, &read_resident_memory);
		collector.process(1'000'000);
		collector.process(2'000'000);
		CHECK(captured_metrics.empty());
		CHECK(resident_memory_reads == 0);
		collector.set_enabled(true);
		collector.process(3'000'000); // establishes a baseline at 3 seconds
		CHECK(captured_metrics.empty());
		CHECK(resident_memory_reads == 0);
		collector.process(4'000'000); // 1s passed since baseline
		CHECK(captured_metrics.size() == 2);
		CHECK(resident_memory_reads == 1);
	}

	TEST_CASE("Emits object and node counts with the expected names, types, values, and units") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		MemoryMetricsCollector collector(fixture.attributes, &read_resident_memory);
		collector.set_enabled(true);
		collector.process(1'000'000); // baseline
		Performance *performance = Performance::get_singleton();
		const double object_count = performance->get_monitor(Performance::OBJECT_COUNT);
		const double node_count = performance->get_monitor(Performance::OBJECT_NODE_COUNT);
		collector.process(2'000'000); // 1s passed since baseline
		REQUIRED_CHECK(captured_metrics.size() == 2);
		Ref<SentryMetric> object_metric = find_metric("game.perf.object_count");
		REQUIRED_CHECK(object_metric.is_valid());
		CHECK(object_metric->get_type() == SentryMetric::METRIC_GAUGE);
		CHECK(object_metric->get_unit().is_empty());
		CHECK(object_metric->get_value() == object_count);
		Ref<SentryMetric> node_metric = find_metric("game.perf.node_count");
		REQUIRED_CHECK(node_metric.is_valid());
		CHECK(node_metric->get_type() == SentryMetric::METRIC_GAUGE);
		CHECK(node_metric->get_unit().is_empty());
		CHECK(node_metric->get_value() == node_count);
	}

	TEST_CASE("Emits positive process resident memory as a byte gauge and skips zero") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		resident_memory_available = true;
		MemoryMetricsCollector collector(fixture.attributes, &read_resident_memory);
		collector.set_enabled(true);

		SUBCASE("Values larger than 32 bits retain their byte value") {
			resident_bytes = 8'000'000'000;
			collector.process(1'000'000); // baseline
			collector.process(2'000'000); // 1s passed since baseline
			CHECK(resident_memory_reads == 1);
			CHECK(find_metric("game.perf.object_count").is_valid());
			CHECK(find_metric("game.perf.node_count").is_valid());
			CHECK(captured_metrics.size() == 3);
			Ref<SentryMetric> metric = find_metric("game.perf.used_memory");
			REQUIRED_CHECK(metric.is_valid());
			CHECK(metric->get_type() == SentryMetric::METRIC_GAUGE);
			CHECK(metric->get_value() == static_cast<double>(resident_bytes));
			CHECK(metric->get_unit() == "byte");
		}
		SUBCASE("Zero is skipped") {
			resident_bytes = 0;
			collector.process(1'000'000); // baseline
			collector.process(2'000'000); // 1s passed since baseline
			CHECK(resident_memory_reads == 1);
			CHECK(find_metric("game.perf.used_memory").is_null());
			CHECK(captured_metrics.size() == 2);
			CHECK(find_metric("game.perf.object_count").is_valid());
			CHECK(find_metric("game.perf.node_count").is_valid());
		}
	}

	TEST_CASE("Attaches the shared attributes to every memory metric") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		resident_memory_available = true;
		resident_bytes = 123'456;
		MemoryMetricsCollector collector(fixture.attributes, &read_resident_memory);
		collector.set_enabled(true);
		collector.process(1'000'000); // baseline
		collector.process(2'000'000); // 1s passed since baseline
		REQUIRED_CHECK(captured_metrics.size() == 3);
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
		MemoryMetricsCollector collector(fixture.attributes, &read_resident_memory);
		collector.set_enabled(true);
		collector.process(1'000'000); // baseline
		collector.process(2'249'999); // just before the reporting deadline
		CHECK(captured_metrics.empty());
		CHECK(resident_memory_reads == 0);
		collector.process(2'250'000); // 1.25s passed since baseline
		CHECK(captured_metrics.size() == 2);
		CHECK(resident_memory_reads == 1);
		collector.process(3'499'999); // just before the next reporting deadline
		CHECK(captured_metrics.size() == 2);
		CHECK(resident_memory_reads == 1);
		collector.process(3'500'000); // 1.25s since last collection
		CHECK(captured_metrics.size() == 4);
		CHECK(resident_memory_reads == 2);
	}

	TEST_CASE("A long stall emits once and starts a new reporting window") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		MemoryMetricsCollector collector(fixture.attributes, &read_resident_memory);
		collector.set_enabled(true);
		collector.process(1'000'000); // baseline
		collector.process(121'000'000); // 120s stall
		CHECK(captured_metrics.size() == 2);
		collector.process(121'999'999); // just before the next reporting deadline
		CHECK(captured_metrics.size() == 2);
		collector.process(122'000'000); // 1s since last collection
		CHECK(captured_metrics.size() == 4);
	}

	TEST_CASE("Reset starts a new reporting interval") {
		constexpr double interval = 1.0;
		InitFixture fixture{ interval };
		REQUIRED_CHECK(SentrySDK::get_singleton()->is_enabled());
		MemoryMetricsCollector collector(fixture.attributes, &read_resident_memory);
		collector.set_enabled(true);
		collector.process(1'000'000); // baseline
		collector.process(1'250'000); // 0.25s passed since baseline
		collector.reset();
		collector.process(10'000'000); // new baseline after reset
		collector.process(10'999'999); // just before the reporting deadline
		CHECK(captured_metrics.empty());
		collector.process(11'000'000); // 1s passed since the new baseline
		CHECK(captured_metrics.size() == 2);
	}
}

#endif // TESTS_ENABLED
