#include "sentry_editor_export_plugin_macos.h"

#ifdef TOOLS_ENABLED

#include "export_utils.h"
#include "gen/sdk_version.gen.h"
#include "sentry/logging/print.h"

using namespace sentry::editor;

String SentryEditorExportPluginMacOS::_get_name() const {
	return "SentryEditorExportPluginMacOS";
}

bool SentryEditorExportPluginMacOS::_supports_platform(const Ref<EditorExportPlatform> &p_platform) const {
	return p_platform->get_os_name() == "macOS";
}

TypedArray<Dictionary> SentryEditorExportPluginMacOS::_get_export_options(const Ref<EditorExportPlatform> &p_platform) const {
	TypedArray<Dictionary> options;
	options.push_back(make_hidden_export_check_option());
	return options;
}

String SentryEditorExportPluginMacOS::_get_export_option_warning(const Ref<EditorExportPlatform> &p_platform, const String &p_option) const {
	if (is_builtin_option_or_hidden_export_option(p_option, "application/min_macos_version")) {
		String min_macos_version = get_option("application/min_macos_version");
		if (min_macos_version.to_float() < SENTRY_MACOS_MIN_VERSION) {
			return vformat("Sentry requires \"Minimum macOS version\" %.1f or higher. Please adjust this setting.", SENTRY_MACOS_MIN_VERSION);
		}
	}
	return String();
}

void SentryEditorExportPluginMacOS::_export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) {
	is_macos = p_features.has("macos");
}

void SentryEditorExportPluginMacOS::_export_end() {
	if (!is_macos) {
		return;
	}
	String min_macos_version = get_option("application/min_macos_version");
	if (min_macos_version.to_float() < SENTRY_MACOS_MIN_VERSION) {
		ERR_PRINT_ED(vformat("Sentry requires \"Minimum macOS version\" to be %.1f or higher. The export completed, but the app may fail to launch on older macOS versions. Please update this export setting before distributing your app.", SENTRY_MACOS_MIN_VERSION));
	}
}

#endif // TOOLS_ENABLED
