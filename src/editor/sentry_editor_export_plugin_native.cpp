#include "sentry_editor_export_plugin_native.h"

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>

String SentryEditorExportPluginNative::_get_name() const {
	return "SentryEditorExportPluginNative";
}

bool SentryEditorExportPluginNative::_supports_platform(const Ref<EditorExportPlatform> &p_platform) const {
	return p_platform->get_os_name() == "Windows" || p_platform->get_os_name() == "Linux";
}

TypedArray<Dictionary> SentryEditorExportPluginNative::_get_export_options(const Ref<EditorExportPlatform> &p_platform) const {
	TypedArray<Dictionary> options;
	Dictionary option;
	option["option"] = Dictionary(PropertyInfo(Variant::INT, "sentry/crash_backend", PROPERTY_HINT_ENUM, "Native,Crashpad"));
	option["default_value"] = 0;
	options.push_back(option);
	return options;
}

void SentryEditorExportPluginNative::_export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) {
	if (!p_features.has("windows") && !p_features.has("linux")) {
		return;
	}

	String arch;
	for (const String &candidate : { String("x86_64"), String("x86_32"), String("arm64") }) {
		if (p_features.has(candidate)) {
			arch = candidate;
			break;
		}
	}
	if (arch.is_empty()) {
		return;
	}

	const bool windows = p_features.has("windows");
	const String platform = windows ? "windows" : "linux";
	const String root = "res://addons/sentry/bin/" + platform + "/" + arch;
	const String linkage_file = root.path_join(p_is_debug ? "linkage.debug.txt" : "linkage.release.txt");
	const bool shared = FileAccess::get_file_as_string(linkage_file).strip_edges() == "shared";
	const bool crashpad = int64_t(get_option("sentry/crash_backend")) == 1;
	const String backend = crashpad ? "crashpad" : "native";
	const String backends_file = root.path_join(p_is_debug ? "backends.debug.txt" : "backends.release.txt");
	const String available_backends = FileAccess::get_file_as_string(backends_file).strip_edges();
	if (available_backends != "both" && available_backends != backend) {
		// A missing dependency makes Godot fail the export instead of silently using a different backend.
		add_shared_object(root.path_join("selected-backend-not-built"), p_features, "");
		ERR_PRINT("Sentry: The selected crash backend is not included in this build. Rebuild with sentry_native_backend=" + backend + " or select an available backend.");
		return;
	}

	const String source_dir = shared ? root.path_join(backend) : root;
	PackedStringArray files;
	if (shared) {
		files.append(windows ? "sentry.dll" : "libsentry.so");
	}
	String handler = crashpad ? "crashpad_handler" : "sentry-crash";
	if (windows) {
		handler += ".exe";
	}
	files.append(handler);
	if (windows) {
		files.append(crashpad ? "crashpad_wer.dll" : "sentry-wer.dll");
	}
	for (const String &file : files) {
		add_shared_object(source_dir.path_join(file), p_features, "");
	}
	add_file("res://addons/sentry/native_backend.txt", backend.to_utf8_buffer(), false);

	// Remove handlers from previous exports so changing backends leaves one matching set.
	const String output_dir = ProjectSettings::get_singleton()->globalize_path(p_path.get_base_dir());
	PackedStringArray stale;
	stale.append(crashpad ? "sentry-crash" : "crashpad_handler");
	if (windows) {
		stale[0] += ".exe";
		stale.append(crashpad ? "sentry-wer.dll" : "crashpad_wer.dll");
	}
	if (!shared) {
		stale.append(windows ? "sentry.dll" : "libsentry.so");
	}
	for (const String &file : stale) {
		const String path = output_dir.path_join(file);
		if (FileAccess::file_exists(path)) {
			ERR_FAIL_COND_MSG(DirAccess::remove_absolute(path) != OK, "Sentry: Cannot remove stale dependency: " + path);
		}
	}
}

#endif // TOOLS_ENABLED
