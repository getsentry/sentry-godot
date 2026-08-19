// Integration tests verifying that option values survive the native-.net-native roundtrip with the interop structs.

#if defined(TESTS_ENABLED)

#include "cpp_test_helpers.h"
#include "dotnet_test_support.h"

#include "sentry/dotnet/csharp_interop.h"
#include "sentry/sentry_options.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <string>

using namespace godot;
using namespace sentry;

using sentry::tests::InitFixture;

namespace {

struct OptionCase {
	// Project setting used by the native layer as the option's initial value.
	const char *setting;

	// Indexed property path on SentryOptions, using ':' for nested sub-objects.
	const char *property;

	// Option value written to the setting, expected to arrive on the managed side.
	Variant from_native;

	// Option value written back by the managed init callback, expected to arrive on the native side.
	Variant from_managed;
};

// The managed harness stores received options by leaf property name.
String option_name(const OptionCase &p_option) {
	const String path = p_option.property;
	return path.substr(path.rfind(":") + 1);
}

} // unnamed namespace

TEST_SUITE("[.NET] Options interop") {
	TEST_CASE("Options crossing the interop boundary") {
		// To cover a new option: add a row here, then report it from InitWithInteropOptions()
		// in DotnetTestHarness.cs, keyed by the last segment of the property path.
		// The two value columns must differ from each other, and "from native" must differ from the
		// option's C# default, or the managed side reports a value that never crossed and the row passes.
		const LocalVector<OptionCase> cases = {
			// clang-format off
			// project setting                                                 property                                        from native                     from managed
			{ "sentry/options/dsn",                                            "dsn",                                          "https://aaa111@127.0.0.1/11",  "https://bbb222@127.0.0.1/22" },
			{ "sentry/options/release",                                        "release",                                      "interop-native-release@1.1.1", "interop-managed-release@2.2.2" },
			{ "sentry/options/dist",                                           "dist",                                         "interop-native-dist",          "interop-managed-dist" },
			{ "sentry/options/environment",                                    "environment",                                  "interop-native-env",           "interop-managed-env" },
			{ "sentry/options/debug_printing",                                 "debug",                                        true,                           false },
			{ "sentry/options/diagnostic_level",                               "diagnostic_level",                             LEVEL_WARNING,                  LEVEL_ERROR },
			{ "sentry/options/sample_rate",                                    "sample_rate",                                  0.5,                            0.75 },
			{ "sentry/options/traces_sample_rate",                             "traces_sample_rate",                           0.6,                            0.8 },
			{ "sentry/options/max_breadcrumbs",                                "max_breadcrumbs",                              11,                             22 },
			{ "sentry/options/shutdown_timeout_ms",                            "shutdown_timeout_ms",                          1100,                           2200 },
			{ "sentry/options/send_default_pii",                               "send_default_pii",                             true,                           false },
			{ "sentry/options/enable_logs",                                    "enable_logs",                                  true,                           false },
			{ "sentry/options/attach_log",                                     "attach_log",                                   false,                          true },
			{ "sentry/options/attach_scene_tree",                              "attach_scene_tree",                            true,                           false },
			{ "sentry/experimental/attach_screenshot",                         "attach_screenshot",                            true,                           false },
			{ "sentry/experimental/screenshot_level",                          "screenshot_level",                             LEVEL_ERROR,                    LEVEL_WARNING },
			{ "sentry/options/app_hang/tracking",                              "enable_app_hang_tracking",                     false,                          true },
			{ "sentry/options/app_hang/timeout_ms",                            "app_hang_timeout_ms",                          1200,                           2300 },
			{ "sentry/options/enable_metrics",                                 "enable_metrics",                               false,                          true },
			{ "sentry/godot_logger/enabled",                                   "godot_logger:enabled",                         false,                          true },
			{ "sentry/godot_logger/include_source_context",                    "godot_logger:include_source_context",          false,                          true },
			{ "sentry/godot_logger/include_variables",                         "godot_logger:include_variables",               true,                           false },
			{ "sentry/godot_logger/events",                                    "godot_logger:event_mask",                      MASK_ERROR | MASK_WARNING,      MASK_SCRIPT | MASK_SHADER },
			{ "sentry/godot_logger/breadcrumbs",                               "godot_logger:breadcrumb_mask",                 MASK_SCRIPT,                    MASK_WARNING },
			{ "sentry/godot_logger/logs",                                      "godot_logger:log_mask",                        MASK_SHADER,                    MASK_ERROR },
			{ "sentry/godot_logger/limits/events_per_frame",                   "godot_logger:limits:events_per_frame",         11,                             21 },
			{ "sentry/godot_logger/limits/repeated_error_window_ms",           "godot_logger:limits:repeated_error_window_ms", 1101,                           2201 },
			{ "sentry/godot_logger/limits/throttle_events",                    "godot_logger:limits:throttle_events",          12,                             22 },
			{ "sentry/godot_logger/limits/throttle_window_ms",                 "godot_logger:limits:throttle_window_ms",       1102,                           2202 },
			{ "sentry/android/application_not_responding/enable_detection",    "android:enable_anr_detection",                 false,                          true },
			{ "sentry/android/application_not_responding/timeout_interval_ms", "android:anr_timeout_interval_ms",              1300,                           2400 },
			{ "sentry/android/application_not_responding/attach_thread_dump",  "android:attach_anr_thread_dump",               true,                           false },
			// clang-format on
		};

		SUBCASE("Every bound option is either crossed or deliberately left out") {
			// Strict options inventory: any new SentryOptions property must be
			// added to the crossing cases or deliberately listed as not crossing.
			HashSet<String> crossed;
			for (const OptionCase &option : cases) {
				crossed.insert(option_name(option));
			}
			const HashSet<String> not_crossed = {
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
					const String name = Dictionary(properties[i])["name"];
					INFO((std::string(class_name) + "." + name.utf8().get_data()));
					CHECK((crossed.has(name) || not_crossed.has(name)));
				}
			}
		}

		SUBCASE("Every option survives the crossing in both directions") {
			if (!sentry::dotnet::godot_supports_dotnet()) {
				MESSAGE("Skipping: managed runtime unavailable (non-mono Godot build).");
				return;
			}

			Dictionary saved;
			for (const OptionCase &option : cases) {
				saved[option.setting] = ProjectSettings::get_singleton()->get_setting(option.setting);
				ProjectSettings::get_singleton()->set_setting(option.setting, option.from_native);
			}
			{
				InitFixture fixture("InitWithInteropOptions"); // inits the SDK, closes at scope exit
				Object *harness = fixture.get_harness();
				CHECK(harness != nullptr);
				CHECK(sentry::dotnet::is_managed_layer_registered());

				if (harness != nullptr) {
					const Dictionary received = harness->call("GetReceivedOptions");
					const Ref<SentryOptions> options = SENTRY_OPTIONS();
					for (const OptionCase &option : cases) {
						INFO(std::string(option_name(option).utf8().get_data()));
						CHECK(received[option_name(option)] == option.from_native);
						CHECK(options->get_indexed(option.property) == option.from_managed);
					}
				}
			}

			for (const OptionCase &option : cases) {
				ProjectSettings::get_singleton()->set_setting(option.setting, saved[option.setting]);
			}
		}
	}
}

#endif // TESTS_ENABLED
