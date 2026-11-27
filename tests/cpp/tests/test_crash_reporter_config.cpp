#if defined(TESTS_ENABLED) && defined(SDK_NATIVE)

#include "cpp_test_helpers.h"
#include "sentry/native/crash_reporter_config.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>

using namespace godot;
using sentry::SentryCrashReporterOptions;
using sentry::crash_reporter::make_app_config;
using sentry::crash_reporter::write_app_config;

TEST_SUITE("Crash reporter config") {
	TEST_CASE("Default options produce an empty config") {
		Ref<SentryCrashReporterOptions> options;
		options.instantiate();
		CHECK(make_app_config(options, String(), String()).is_empty());
	}

	TEST_CASE("Set options map to the reporter's config keys") {
		Ref<SentryCrashReporterOptions> options;
		options.instantiate();
		options->set_window_title("Game Crashed");
		options->set_header_text("Sorry!");
		options->set_header_description("Tell us what happened.");
		options->set_submit_button_label("Send");
		options->set_cancel_button_label("Skip");
		options->set_accent_color(Color(1.0f, 0.5f, 0.0f));
		options->set_window_closable(false);

		const Dictionary config = make_app_config(options, "/game/light.png", "/game/dark.png");
		CHECK(config["WindowTitle"] == "Game Crashed");
		CHECK(config["HeaderText"] == "Sorry!");
		CHECK(config["HeaderDescription"] == "Tell us what happened.");
		CHECK(config["SubmitButton"] == "Send");
		CHECK(config["CancelButton"] == "Skip");
		CHECK(config["SystemAccentColor"] == "#FF8000");
		CHECK(config["LogoLight"] == "/game/light.png");
		CHECK(config["LogoDark"] == "/game/dark.png");
		CHECK(config["WindowClosable"] == Variant(false));
		CHECK(config.size() == 9);
	}

	TEST_CASE("Accent color with any alpha above zero is written without alpha") {
		Ref<SentryCrashReporterOptions> options;
		options.instantiate();
		options->set_accent_color(Color(1.0f, 0.5f, 0.0f, 0.3f));

		const Dictionary config = make_app_config(options, String(), String());
		CHECK(config["SystemAccentColor"] == "#FF8000");
	}

	TEST_CASE("Each view maps to the reporter's name for it") {
		Ref<SentryCrashReporterOptions> options;
		options.instantiate();
		TypedArray<int> views;
		views.push_back(SentryCrashReporterOptions::VIEW_FEEDBACK);
		views.push_back(SentryCrashReporterOptions::VIEW_TAGS);
		views.push_back(SentryCrashReporterOptions::VIEW_CONTEXTS);
		views.push_back(SentryCrashReporterOptions::VIEW_EXTRA);
		views.push_back(SentryCrashReporterOptions::VIEW_SDK);
		views.push_back(SentryCrashReporterOptions::VIEW_USER);
		views.push_back(SentryCrashReporterOptions::VIEW_ATTACHMENTS);
		views.push_back(SentryCrashReporterOptions::VIEW_STACKTRACE);
		views.push_back(SentryCrashReporterOptions::VIEW_ENVELOPE);
		options->set_views(views);

		const Dictionary config = make_app_config(options, String(), String());
		CHECK(config["Views"] == PackedStringArray({ "feedback", "tags", "contexts", "extra", "sdk", "user", "attachments", "stacktrace", "envelope" }));
	}

	TEST_CASE("Views keep their order and drop duplicates and unknown values") {
		Ref<SentryCrashReporterOptions> options;
		options.instantiate();
		TypedArray<int> views;
		views.push_back(SentryCrashReporterOptions::VIEW_STACKTRACE);
		views.push_back(SentryCrashReporterOptions::VIEW_FEEDBACK);
		views.push_back(SentryCrashReporterOptions::VIEW_STACKTRACE);
		views.push_back(99);
		views.push_back(SentryCrashReporterOptions::VIEW_SDK);
		options->set_views(views);

		const Dictionary config = make_app_config(options, String(), String());
		CHECK(config["Views"] == PackedStringArray({ "stacktrace", "feedback", "sdk" }));
		CHECK(config.size() == 1);
	}

	TEST_CASE("Config file is written under AppConfig") {
		const String dir = OS::get_singleton()->get_user_data_dir().path_join("test_crash_reporter_config");
		const String path = dir.path_join("appsettings.json");

		Dictionary config;
		config["WindowTitle"] = "Game Crashed";
		write_app_config(config, dir);
		REQUIRED_CHECK(FileAccess::file_exists(path));
		const Dictionary root = JSON::parse_string(FileAccess::get_file_as_string(path));
		CHECK(Dictionary(root["AppConfig"])["WindowTitle"] == "Game Crashed");

		DirAccess::remove_absolute(path);
		DirAccess::remove_absolute(dir);
	}

	TEST_CASE("Config file is removed when the config is empty") {
		const String dir = OS::get_singleton()->get_user_data_dir().path_join("test_crash_reporter_config");
		const String path = dir.path_join("appsettings.json");

		Dictionary config;
		config["WindowTitle"] = "Game Crashed";
		write_app_config(config, dir);
		REQUIRED_CHECK(FileAccess::file_exists(path));

		write_app_config(Dictionary(), dir);
		CHECK_FALSE(FileAccess::file_exists(path));

		DirAccess::remove_absolute(dir);
	}
}

#endif // TESTS_ENABLED && SDK_NATIVE
