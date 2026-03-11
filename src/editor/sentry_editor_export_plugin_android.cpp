#include "sentry_editor_export_plugin_android.h"

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_export_platform.hpp>

String SentryEditorExportPluginAndroid::_get_name() const {
	return "SentryAndroidExportPlugin";
}

bool SentryEditorExportPluginAndroid::_supports_platform(const Ref<EditorExportPlatform> &p_platform) const {
	return p_platform->get_os_name() == "Android";
}

PackedStringArray SentryEditorExportPluginAndroid::_get_android_libraries(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const {
	PackedStringArray libs;
	String release_or_debug = p_debug ? "debug" : "release";
	libs.append("sentry/bin/android/sentry_android_godot_plugin." + release_or_debug + ".aar");
	return libs;
}

PackedStringArray SentryEditorExportPluginAndroid::_get_android_dependencies(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const {
	PackedStringArray deps;
	// NOTE: All dependencies below must be also updated in build.gradle.kts.
	deps.append("io.sentry:sentry-android:8.34.1");
	return deps;
}

TypedArray<Dictionary> SentryEditorExportPluginAndroid::_get_export_options(const Ref<EditorExportPlatform> &p_platform) const {
	TypedArray<Dictionary> options;
	Dictionary option;
	// HACK: Adding a hidden option so we can show a warning at the bottom of the export dialog.
	option["option"] = Dictionary(PropertyInfo(
			Variant::BOOL,
			"sentry/_export_check",
			PROPERTY_HINT_NONE,
			String(),
			PROPERTY_USAGE_NONE));
	option["default_value"] = true;
	options.push_back(option);
	return options;
}

String SentryEditorExportPluginAndroid::_get_export_option_warning(const Ref<EditorExportPlatform> &p_platform, const String &p_option) const {
	// HACK: Also check "sentry/_export_check" so the warning appears at the bottom of the export dialog.
	//       Godot shows bottom-bar warnings through a separate code path that only checks plugin-owned options.
	if ((p_option == "gradle_build/use_gradle_build" || p_option == "sentry/_export_check") &&
			get_option("gradle_build/use_gradle_build") == Variant(false)) {
		return "Sentry requires \"Use Gradle Build\" to be enabled for Android exports.";
	}
	return String();
}

#endif // TOOLS_ENABLED
