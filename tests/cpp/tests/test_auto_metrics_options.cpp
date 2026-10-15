// Defaults are tested in GDScript.

#ifdef TESTS_ENABLED

#include "cpp_test_helpers.h"
#include "sentry/sentry_options.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;
using namespace sentry;

TEST_SUITE("Automatic metrics options") {
	TEST_CASE("Normalized collection intervals are clamped and converted to microseconds") {
		Ref<SentryAutoMetricsOptions> options;
		options.instantiate();
		const struct {
			double seconds;
			uint64_t microseconds;
		} cases[] = {
			{ -1.0, 1'000'000 },
			{ 0.0, 1'000'000 },
			{ 0.5, 1'000'000 },
			{ 1.0, 1'000'000 },
			{ 1.25, 1'250'000 },
			{ 86'400.0, 86'400'000'000 },
			{ 86'401.0, 86'400'000'000 },
			{ 1e308, 86'400'000'000 },
			{ Math::NaN, 1'000'000 },
			{ Math::INF, 1'000'000 },
			{ -Math::INF, 1'000'000 },
		};
		for (const auto &test : cases) {
			CAPTURE(test.seconds);
			options->set_frame_metrics_interval_sec(test.seconds);
			CHECK(options->get_normalized_frame_metrics_interval_usec() == test.microseconds);
			options->set_rendering_metrics_interval_sec(test.seconds);
			CHECK(options->get_normalized_rendering_metrics_interval_usec() == test.microseconds);
		}
	}

	TEST_CASE("Normalized interval getters read their own category's option") {
		Ref<SentryAutoMetricsOptions> options;
		options.instantiate();
		options->set_frame_metrics_interval_sec(2.0);
		options->set_rendering_metrics_interval_sec(1.25);
		CHECK(options->get_normalized_frame_metrics_interval_usec() == 2'000'000);
		CHECK(options->get_normalized_rendering_metrics_interval_usec() == 1'250'000);
	}
}

#endif // TESTS_ENABLED
