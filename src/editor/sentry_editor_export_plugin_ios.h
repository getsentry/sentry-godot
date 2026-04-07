#pragma once

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_export_platform.hpp>
#include <godot_cpp/classes/editor_export_plugin.hpp>

using namespace godot;

class SentryEditorExportPluginIOS : public EditorExportPlugin {
	GDCLASS(SentryEditorExportPluginIOS, EditorExportPlugin);

private:
	bool is_ios = false;

protected:
	static void _bind_methods() {}

public:
	virtual String _get_name() const override;
	virtual bool _supports_platform(const Ref<EditorExportPlatform> &p_platform) const override;
	virtual TypedArray<Dictionary> _get_export_options(const Ref<EditorExportPlatform> &p_platform) const override;
	virtual String _get_export_option_warning(const Ref<EditorExportPlatform> &p_platform, const String &p_option) const override;
	virtual void _export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) override;
	virtual void _export_end() override;

	SentryEditorExportPluginIOS() = default;
};

#endif // TOOLS_ENABLED
