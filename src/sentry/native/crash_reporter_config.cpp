#include "crash_reporter_config.h"

#include "sentry/logging/print.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>

using namespace godot;

namespace {

// Names the reporter uses for its views, indexed by SentryCrashReporterOptions::View.
constexpr const char *VIEW_NAMES[] = {
	"feedback",
	"tags",
	"contexts",
	"extra",
	"sdk",
	"user",
	"attachments",
	"stacktrace",
	"envelope",
};
constexpr int VIEW_COUNT = sizeof(VIEW_NAMES) / sizeof(VIEW_NAMES[0]);

void _set_if_not_empty(Dictionary &r_config, const char *p_key, const String &p_value) {
	if (!p_value.is_empty()) {
		r_config[p_key] = p_value;
	}
}

} // unnamed namespace

namespace sentry::crash_reporter {

Dictionary make_app_config(const Ref<SentryCrashReporterOptions> &p_options, const String &p_logo_light_theme_path, const String &p_logo_dark_theme_path) {
	Dictionary config;
	_set_if_not_empty(config, "WindowTitle", p_options->get_window_title());
	_set_if_not_empty(config, "HeaderText", p_options->get_header_text());
	_set_if_not_empty(config, "HeaderDescription", p_options->get_header_description());
	_set_if_not_empty(config, "SubmitButton", p_options->get_submit_button_label());
	_set_if_not_empty(config, "CancelButton", p_options->get_cancel_button_label());

	const Color accent_color = p_options->get_accent_color();
	if (accent_color.a > 0.0f) {
		config["SystemAccentColor"] = "#" + accent_color.to_html(false).to_upper();
	}

	_set_if_not_empty(config, "LogoLight", p_logo_light_theme_path);
	_set_if_not_empty(config, "LogoDark", p_logo_dark_theme_path);

	if (!p_options->get_window_closable()) {
		config["WindowClosable"] = false;
	}

	PackedStringArray views;
	const TypedArray<int> view_ids = p_options->get_views();
	for (int i = 0; i < view_ids.size(); i++) {
		const int view = view_ids[i];
		if (view < 0 || view >= VIEW_COUNT) {
			sentry::logging::print_warning(vformat("Ignoring unknown crash reporter view: %d", view));
			continue;
		}
		const String view_name{ VIEW_NAMES[view] };
		if (!views.has(view_name)) {
			views.push_back(view_name);
		}
	}
	if (!views.is_empty()) {
		config["Views"] = views;
	}

	return config;
}

void write_app_config(const Dictionary &p_config, const String &p_dir) {
	const String path = p_dir.path_join("appsettings.json");

	if (p_config.is_empty()) {
		if (FileAccess::file_exists(path)) {
			DirAccess::remove_absolute(path);
		}
		return;
	}

	DirAccess::make_dir_recursive_absolute(p_dir);
	Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
	if (file.is_null()) {
		sentry::logging::print_warning("Failed to write crash reporter config: " + path);
		return;
	}

	Dictionary root;
	root["AppConfig"] = p_config;
	file->store_string(JSON::stringify(root, "\t"));
}

} // namespace sentry::crash_reporter
