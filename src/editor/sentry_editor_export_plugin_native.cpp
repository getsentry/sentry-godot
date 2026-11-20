#include "sentry_editor_export_plugin_native.h"

#ifdef TOOLS_ENABLED

#include "sentry/sentry_options.h"

#include <godot_cpp/classes/editor_export_preset.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include <initializer_list>

using sentry::SentryCrashReporterOptions;

namespace {

constexpr const char *BACKEND_OPTION = "sentry/crash_backend";
constexpr const char *FEATURE_TAG_NATIVE = "sentry_crash_backend_native";
constexpr const char *FEATURE_TAG_CRASHPAD = "sentry_crash_backend_crashpad";

constexpr std::initializer_list<const char *> NATIVE_LINUX_DEPENDENCIES = {
	"sentry-crash",
};
constexpr std::initializer_list<const char *> NATIVE_WINDOWS_DEPENDENCIES = {
	"sentry-crash.exe",
	"sentry-wer.dll",
};
constexpr std::initializer_list<const char *> CRASHPAD_LINUX_DEPENDENCIES = {
	"crashpad_handler",
};
constexpr std::initializer_list<const char *> CRASHPAD_WINDOWS_DEPENDENCIES = {
	"crashpad_handler.exe",
	"crashpad_wer.dll",
};

} // namespace

String SentryEditorExportPluginNative::_get_name() const {
	return "SentryEditorExportPluginNative";
}

bool SentryEditorExportPluginNative::_supports_platform(const Ref<EditorExportPlatform> &p_platform) const {
	return p_platform->get_os_name() == "Linux" || p_platform->get_os_name() == "Windows";
}

TypedArray<Dictionary> SentryEditorExportPluginNative::_get_export_options(const Ref<EditorExportPlatform> &p_platform) const {
	Dictionary option;
	option["option"] = Dictionary(PropertyInfo(Variant::INT, BACKEND_OPTION, PROPERTY_HINT_ENUM, "Auto:0,Native:1,Crashpad:2"));
	option["default_value"] = static_cast<int>(CrashBackend::AUTO);
	TypedArray<Dictionary> options;
	options.push_back(option);
	return options;
}

SentryEditorExportPluginNative::CrashBackend SentryEditorExportPluginNative::_resolve_crash_backend() const {
	// Export option takes precedence over preset feature tags.
	Variant option = get_option(BACKEND_OPTION);
	CrashBackend backend = option.get_type() == Variant::NIL ? CrashBackend::AUTO : static_cast<CrashBackend>(int(option));
	if (backend == CrashBackend::NATIVE || backend == CrashBackend::CRASHPAD) {
		return backend;
	}

	// Export preset feature tags take precedence over the SDK default.
	Ref<EditorExportPreset> preset = get_export_preset();
	if (preset.is_valid()) {
		PackedStringArray features = preset->get_custom_features().split(",", false);
		for (int i = 0; i < features.size(); i++) {
			features[i] = features[i].strip_edges();
		}
		if (features.has(FEATURE_TAG_CRASHPAD)) {
			return CrashBackend::CRASHPAD;
		}
		if (features.has(FEATURE_TAG_NATIVE)) {
			return CrashBackend::NATIVE;
		}
	}

	return DEFAULT_BACKEND;
}

PackedStringArray SentryEditorExportPluginNative::_get_export_features(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const {
	PackedStringArray features;
	if (_supports_platform(p_platform)) {
		features.push_back(_resolve_crash_backend() == CrashBackend::NATIVE ? FEATURE_TAG_NATIVE : FEATURE_TAG_CRASHPAD);
	}
	return features;
}

void SentryEditorExportPluginNative::_export_file(const String &p_path, const String &p_type, const PackedStringArray &p_features) {
	if (p_path != "res://addons/sentry/sentry.gdextension") {
		return;
	}

	const bool is_windows = p_features.has("windows");
	const bool is_linux = p_features.has("linux");

	if (!is_windows && !is_linux) {
		return;
	}

	const String platform = is_windows ? "windows" : "linux";
	const String arch = get_option("binary_format/architecture");
	const CrashBackend backend = _resolve_crash_backend();
	const String source_dir = vformat("res://addons/sentry/bin/%s/%s/%s", platform, arch, backend == CrashBackend::NATIVE ? "native" : "crashpad");
	const String extension = vformat("libsentry.%s.%s.%s.%s",
			platform,
			p_features.has("debug") ? "debug" : "release",
			arch,
			is_windows ? "dll" : "so");
	const auto dependencies = is_windows
			? (backend == CrashBackend::NATIVE ? NATIVE_WINDOWS_DEPENDENCIES : CRASHPAD_WINDOWS_DEPENDENCIES)
			: (backend == CrashBackend::NATIVE ? NATIVE_LINUX_DEPENDENCIES : CRASHPAD_LINUX_DEPENDENCIES);

	// WORKAROUND: Queue these after Godot's GDExtension exporter so the selected variant replaces its copy.
	// WHY: Godot does not use _get_export_features() tags to select GDExtension libraries (an oversight?),
	// so we resolve and queue the proper backend library explicitly.

	add_shared_object(source_dir.path_join(extension), PackedStringArray(), String());
	for (const char *dependency : dependencies) {
		add_shared_object(source_dir.path_join(dependency), PackedStringArray(), String());
	}

	_export_crash_reporter(platform, arch);
}

void SentryEditorExportPluginNative::_export_crash_reporter(const String &p_platform, const String &p_arch) {
	Ref<EditorExportPreset> preset = get_export_preset();
	ERR_FAIL_COND(preset.is_null());
	const bool is_enabled = preset->get_project_setting("sentry/crash_reporter/enabled").operator bool();
	if (!is_enabled) {
		return;
	}

	Ref<EditorExportPlatform> export_platform = get_export_platform();
	ERR_FAIL_COND(export_platform.is_null());

	const String path_setting = SentryCrashReporterOptions::get_path_setting(p_platform, p_arch);
	// Architectures without a path setting have no reporter.
	const String path = ProjectSettings::get_singleton()->has_setting(path_setting) ? String(preset->get_project_setting(path_setting)) : String();
	if (path.is_empty()) {
		export_platform->add_message(EditorExportPlatform::EXPORT_MESSAGE_WARNING, "Sentry",
				vformat("Crash reporter is enabled, but \"%s\" is empty. Exporting without the crash reporter.", path_setting));
		return;
	}

	if (!FileAccess::file_exists(path)) {
		String message = vformat("Crash reporter not found at \"%s\".", path);
		if (path == SentryCrashReporterOptions::get_default_path(p_platform, p_arch)) {
			message += " Install the Sentry crash reporter package (sentry-godot-crash-reporter-<version>.zip) from the SDK release, or disable the crash reporter.";
		} else {
			message += vformat(" Update \"%s\" in the project settings, or disable the crash reporter.", path_setting);
		}
		export_platform->add_message(EditorExportPlatform::EXPORT_MESSAGE_ERROR, "Sentry", message);
		return;
	}

	add_shared_object(path, PackedStringArray(), String());
}

#endif // TOOLS_ENABLED
