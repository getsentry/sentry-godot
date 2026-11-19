#pragma once

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_export_platform.hpp>
#include <godot_cpp/classes/editor_export_plugin.hpp>

using namespace godot;

// Exports the selected crash backend and its dependencies on Linux and Windows.
class SentryEditorExportPluginNative : public EditorExportPlugin {
	GDCLASS(SentryEditorExportPluginNative, EditorExportPlugin);

private:
	enum class CrashBackend {
		AUTO = 0,
		NATIVE = 1,
		CRASHPAD = 2,
	};
	static constexpr CrashBackend DEFAULT_BACKEND = CrashBackend::CRASHPAD;

	String _export_path;
	String _exported_reporter;

	CrashBackend _resolve_crash_backend() const;
	void _export_crash_reporter(const String &p_platform, const String &p_arch);

protected:
	static void _bind_methods() {}

public:
	virtual String _get_name() const override;
	virtual bool _supports_platform(const Ref<EditorExportPlatform> &p_platform) const override;
	virtual TypedArray<Dictionary> _get_export_options(const Ref<EditorExportPlatform> &p_platform) const override;
	virtual PackedStringArray _get_export_features(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const override;
	virtual void _export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) override;
	virtual void _export_end() override;
	virtual void _export_file(const String &p_path, const String &p_type, const PackedStringArray &p_features) override;
};

#endif // TOOLS_ENABLED
