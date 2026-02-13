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
	deps.append("io.sentry:sentry-android:8.32.0");
	return deps;
}

#endif // TOOLS_ENABLED
