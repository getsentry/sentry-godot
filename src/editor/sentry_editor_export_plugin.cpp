#include "sentry_editor_export_plugin.h"

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_export_platform.hpp>

String SentryEditorExportPlugin::_get_name() const {
	return "SentryAndroidExportPlugin";
}

bool SentryEditorExportPlugin::_supports_platform(const Ref<EditorExportPlatform> &p_platform) const {
	return p_platform->is_class("EditorExportPlatformAndroid");
}

PackedStringArray SentryEditorExportPlugin::_get_android_libraries(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const {
	PackedStringArray libs;
	String release_or_debug = p_debug ? "debug" : "release";
	libs.append("sentry/bin/android/sentry_android_godot_plugin." + release_or_debug + ".aar");
	return libs;
}

PackedStringArray SentryEditorExportPlugin::_get_android_dependencies(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const {
	PackedStringArray deps;
	// Note: Must be also updated in build.gradle.kts.
	deps.append("io.sentry:sentry-android:8.13.2");
	deps.append("com.jakewharton.threetenabp:threetenabp:1.4.9");
	return deps;
}

#endif // TOOLS_ENABLED
