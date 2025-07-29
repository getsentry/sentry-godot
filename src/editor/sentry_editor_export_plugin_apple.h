#ifndef SENTRY_EDITOR_EXPORT_PLUGIN_APPLE_H
#define SENTRY_EDITOR_EXPORT_PLUGIN_APPLE_H

#if defined(TOOLS_ENABLED)

#include <godot_cpp/classes/editor_export_platform.hpp>
#include <godot_cpp/classes/editor_export_plugin.hpp>

using namespace godot;

class SentryEditorExportPluginApple : public EditorExportPlugin {
	GDCLASS(SentryEditorExportPluginApple, EditorExportPlugin);

private:
	String export_path;

protected:
	static void _bind_methods() {}

public:
	virtual String _get_name() const override;
	virtual bool _supports_platform(const Ref<EditorExportPlatform> &p_platform) const override;
	virtual void _export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) override;
	virtual void _export_end() override;
};

#endif // TOOLS_ENABLED

#endif // SENTRY_EDITOR_EXPORT_PLUGIN_APPLE_H
