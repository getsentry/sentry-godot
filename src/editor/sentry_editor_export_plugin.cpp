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
	libs.append("sentrysdk/bin/android/sentryandroidgodotplugin.aar");
	return libs;
}

PackedStringArray SentryEditorExportPlugin::_get_android_dependencies(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const {
	PackedStringArray deps;
	deps.append("io.sentry:sentry-android:8.8.0");
	deps.append("com.jakewharton.threetenabp:threetenabp:1.4.9");
	return deps;
}

#endif // TOOLS_ENABLED
