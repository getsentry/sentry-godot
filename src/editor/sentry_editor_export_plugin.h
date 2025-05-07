#ifndef SENTRY_EDITOR_EXPORT_PLUGIN_H
#define SENTRY_EDITOR_EXPORT_PLUGIN_H

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_export_platform.hpp>
#include <godot_cpp/classes/editor_export_plugin.hpp>

using namespace godot;

class SentryEditorExportPlugin : public EditorExportPlugin {
	GDCLASS(SentryEditorExportPlugin, EditorExportPlugin);

protected:
	static void _bind_methods() {}

public:
	virtual String _get_name() const override;
	virtual bool _supports_platform(const Ref<EditorExportPlatform> &p_platform) const override;
	virtual PackedStringArray _get_android_libraries(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const override;
	virtual PackedStringArray _get_android_dependencies(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const override;
};

#endif // TOOLS_ENABLED

#endif // SENTRY_EDITOR_EXPORT_PLUGIN_H
