// Integration tests verifying that option values survive the native-.net-native roundtrip with the interop structs.

#if defined(TESTS_ENABLED)

#include "cpp_test_helpers.h"
#include "dotnet_test_support.h"

#include "sentry/dotnet/csharp_interop.h"
#include "sentry/sentry_options.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <set>
#include <string>

using namespace godot;
using namespace sentry;

using sentry::tests::InitFixture;

TEST_SUITE("[.NET] Options interop") {
	TEST_CASE("Every option survives the crossing in both directions") {
		if (!sentry::dotnet::godot_supports_dotnet()) {
			MESSAGE("Skipping: managed runtime unavailable (non-mono Godot build).");
			return;
		}

		// The project settings written here travel native -> managed, and the managed callback
		// writes a different value to every option on the way back. No same value appears on both
		// sides, so an option can only match if it actually crossed.
		Dictionary saved;
		auto write_setting = [&saved](const String &p_key, const Variant &p_value) {
			saved[p_key] = ProjectSettings::get_singleton()->get_setting(p_key);
			ProjectSettings::get_singleton()->set_setting(p_key, p_value);
		};

		write_setting("sentry/options/dsn", "https://aaa111@127.0.0.1/11");
		write_setting("sentry/options/release", "interop-native-release@1.1.1");
		write_setting("sentry/options/dist", "interop-native-dist");
		write_setting("sentry/options/environment", "interop-native-env");
		write_setting("sentry/options/debug_printing", 1); // On
		write_setting("sentry/options/diagnostic_level", sentry::LEVEL_WARNING);
		write_setting("sentry/options/sample_rate", 0.5);
		write_setting("sentry/options/traces_sample_rate", 0.6);
		write_setting("sentry/options/max_breadcrumbs", 11);
		write_setting("sentry/options/shutdown_timeout_ms", 1100);
		write_setting("sentry/options/send_default_pii", true);
		write_setting("sentry/options/enable_logs", false);
		write_setting("sentry/options/attach_log", false);
		write_setting("sentry/options/attach_scene_tree", false);
		write_setting("sentry/experimental/attach_screenshot", true);
		write_setting("sentry/experimental/screenshot_level", sentry::LEVEL_ERROR);
		write_setting("sentry/options/app_hang/tracking", false);
		write_setting("sentry/options/app_hang/timeout_ms", 1200);
		write_setting("sentry/options/enable_metrics", false);
		write_setting("sentry/godot_logger/enabled", false);
		write_setting("sentry/godot_logger/include_source_context", false);
		write_setting("sentry/godot_logger/include_variables", false);
		write_setting("sentry/godot_logger/events", GodotLoggerEventMask::MASK_ERROR | GodotLoggerEventMask::MASK_WARNING);
		write_setting("sentry/godot_logger/breadcrumbs", GodotLoggerEventMask::MASK_SCRIPT);
		write_setting("sentry/godot_logger/logs", GodotLoggerEventMask::MASK_SHADER);
		write_setting("sentry/godot_logger/limits/events_per_frame", 11);
		write_setting("sentry/godot_logger/limits/repeated_error_window_ms", 1101);
		write_setting("sentry/godot_logger/limits/throttle_events", 12);
		write_setting("sentry/godot_logger/limits/throttle_window_ms", 1102);
		write_setting("sentry/android/application_not_responding/enable_detection", false);
		write_setting("sentry/android/application_not_responding/timeout_interval_ms", 1300);
		write_setting("sentry/android/application_not_responding/attach_thread_dump", true);

		{
			InitFixture fixture("InitWithInteropOptions"); // inits the SDK, closes at scope exit
			Object *harness = fixture.get_harness();
			CHECK(harness != nullptr);
			CHECK(sentry::dotnet::is_managed_layer_registered());

			if (harness != nullptr) {
				// Native options reached the managed layer.
				const Dictionary received = harness->call("GetReceivedOptions");
				CHECK(String(received["dsn"]) == "https://aaa111@127.0.0.1/11");
				CHECK(String(received["release"]) == "interop-native-release@1.1.1");
				CHECK(String(received["dist"]) == "interop-native-dist");
				CHECK(String(received["environment"]) == "interop-native-env");
				CHECK(bool(received["debug"]) == true);
				CHECK(int(received["diagnostic_level"]) == sentry::LEVEL_WARNING);
				CHECK(double(received["sample_rate"]) == doctest::Approx(0.5));
				CHECK(double(received["traces_sample_rate"]) == doctest::Approx(0.6));
				CHECK(int(received["max_breadcrumbs"]) == 11);
				CHECK(double(received["shutdown_timeout_ms"]) == doctest::Approx(1100));
				CHECK(bool(received["send_default_pii"]) == true);
				CHECK(bool(received["enable_logs"]) == false);
				CHECK(bool(received["attach_log"]) == false);
				CHECK(bool(received["attach_scene_tree"]) == false);
				CHECK(bool(received["attach_screenshot"]) == true);
				CHECK(int(received["screenshot_level"]) == sentry::LEVEL_ERROR);
				CHECK(bool(received["enable_app_hang_tracking"]) == false);
				CHECK(double(received["app_hang_timeout_ms"]) == doctest::Approx(1200));
				CHECK(bool(received["enable_metrics"]) == false);
				CHECK(bool(received["logger_enabled"]) == false);
				CHECK(bool(received["logger_include_source_context"]) == false);
				CHECK(bool(received["logger_include_variables"]) == false);
				CHECK(int(received["logger_event_mask"]) == (GodotLoggerEventMask::MASK_ERROR | GodotLoggerEventMask::MASK_WARNING));
				CHECK(int(received["logger_breadcrumb_mask"]) == GodotLoggerEventMask::MASK_SCRIPT);
				CHECK(int(received["logger_log_mask"]) == GodotLoggerEventMask::MASK_SHADER);
				CHECK(int(received["events_per_frame"]) == 11);
				CHECK(double(received["repeated_error_window_ms"]) == doctest::Approx(1101));
				CHECK(int(received["throttle_events"]) == 12);
				CHECK(double(received["throttle_window_ms"]) == doctest::Approx(1102));
				CHECK(bool(received["enable_anr_detection"]) == false);
				CHECK(double(received["anr_timeout_interval_ms"]) == doctest::Approx(1300));
				CHECK(bool(received["attach_anr_thread_dump"]) == true);

				// Managed options reached the native layer.
				const Ref<SentryOptions> options = SENTRY_OPTIONS();
				CHECK(options->get_dsn() == "https://bbb222@127.0.0.1/22");
				CHECK(options->get_release() == "interop-managed-release@2.2.2");
				CHECK(options->get_dist() == "interop-managed-dist");
				CHECK(options->get_environment() == "interop-managed-env");
				CHECK(options->is_debug_enabled() == false);
				CHECK(options->get_diagnostic_level() == sentry::LEVEL_ERROR);
				CHECK(options->get_sample_rate() == doctest::Approx(0.75));
				CHECK(options->get_traces_sample_rate() == doctest::Approx(0.8));
				CHECK(options->get_max_breadcrumbs() == 22);
				CHECK(options->get_shutdown_timeout_ms() == 2200);
				CHECK(options->is_send_default_pii_enabled() == false);
				CHECK(options->get_enable_logs() == true);
				CHECK(options->is_attach_log_enabled() == true);
				CHECK(options->is_attach_scene_tree_enabled() == true);
				CHECK(options->is_attach_screenshot_enabled() == false);
				CHECK(options->get_screenshot_level() == sentry::LEVEL_WARNING);
				CHECK(options->is_app_hang_tracking_enabled() == true);
				CHECK(options->get_app_hang_timeout_ms() == 2300);
				CHECK(options->get_enable_metrics() == true);
				CHECK(options->get_godot_logger()->get_enabled() == true);
				CHECK(options->get_godot_logger()->get_include_source_context() == true);
				CHECK(options->get_godot_logger()->get_include_variables() == true);
				CHECK(options->get_godot_logger()->get_event_mask() == (GodotLoggerEventMask::MASK_SCRIPT | GodotLoggerEventMask::MASK_SHADER));
				CHECK(options->get_godot_logger()->get_breadcrumb_mask() == GodotLoggerEventMask::MASK_WARNING);
				CHECK(options->get_godot_logger()->get_log_mask() == GodotLoggerEventMask::MASK_ERROR);
				CHECK(options->get_godot_logger()->get_limits()->get_events_per_frame() == 21);
				CHECK(options->get_godot_logger()->get_limits()->get_repeated_error_window_ms() == 2201);
				CHECK(options->get_godot_logger()->get_limits()->get_throttle_events() == 22);
				CHECK(options->get_godot_logger()->get_limits()->get_throttle_window_ms() == 2202);
				CHECK(options->get_android()->get_enable_anr_detection() == true);
				CHECK(options->get_android()->get_anr_timeout_interval_ms() == 2400);
				CHECK(options->get_android()->get_attach_anr_thread_dump() == false);
			}
		}

		const Array saved_keys = saved.keys();
		for (int i = 0; i < saved_keys.size(); i++) {
			ProjectSettings::get_singleton()->set_setting(saved_keys[i], saved[saved_keys[i]]);
		}
	}

	TEST_CASE("Every bound option is either crossed or deliberately left out") {
		// An option added to SentryOptions without a decision about the interop structs fails
		// here, which is the only place the two sides can be compared against a full inventory.
		const std::set<std::string> crossed = {
			// SentryOptions
			"dsn", "release", "dist", "environment", "debug", "diagnostic_level",
			"sample_rate", "traces_sample_rate", "max_breadcrumbs", "shutdown_timeout_ms",
			"send_default_pii", "attach_log", "attach_screenshot", "screenshot_level",
			"attach_scene_tree", "enable_logs", "enable_metrics",
			"enable_app_hang_tracking", "app_hang_timeout_ms",
			// SentryGodotLoggerOptions
			"enabled", "include_source_context", "include_variables",
			"event_mask", "breadcrumb_mask", "log_mask",
			// SentryLoggerLimits
			"events_per_frame", "repeated_error_window_ms", "throttle_events", "throttle_window_ms",
			// SentryAndroidOptions
			"enable_anr_detection", "anr_timeout_interval_ms", "attach_anr_thread_dump"
		};
		const std::set<std::string> not_crossed = {
			// Callables, which each layer keeps to itself.
			"before_send", "before_send_log", "before_send_metric", "before_capture_screenshot",
			// Sub-objects, whose own properties cross instead.
			"experimental", "android", "godot_logger", "limits",
			// Deprecated aliases for the properties above.
			"logger_messages_as_breadcrumbs", "app_hang_tracking", "app_hang_timeout_sec",
			"logger_enabled", "logger_include_source", "logger_include_variables",
			"logger_event_mask", "logger_breadcrumb_mask", "logger_log_mask", "logger_limits"
		};

		for (const char *class_name : { "SentryOptions", "SentryGodotLoggerOptions", "SentryLoggerLimits", "SentryAndroidOptions" }) {
			const TypedArray<Dictionary> properties = ClassDBSingleton::get_singleton()->class_get_property_list(class_name, true);
			for (int i = 0; i < properties.size(); i++) {
				const std::string name = String(Dictionary(properties[i])["name"]).utf8().get_data();
				INFO(class_name << "." << name);
				CHECK((crossed.count(name) > 0 || not_crossed.count(name) > 0));
			}
		}
	}
}

#endif // TESTS_ENABLED
