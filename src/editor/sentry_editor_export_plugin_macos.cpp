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

bool SentryEditorExportPluginMacOS::_has_incompatible_minimum_version() const {
	String architecture = get_option("binary_format/architecture");
	String min_macos_version_intel = get_option("application/min_macos_version_x86_64");
	String min_macos_version_arm = get_option("application/min_macos_version_arm64");
	bool exports_intel = architecture == "universal" || architecture == "x86_64";
	bool exports_arm = architecture == "universal" || architecture == "arm64";
	return (exports_intel && min_macos_version_intel.to_float() < SENTRY_MACOS_MIN_VERSION) ||
			(exports_arm && min_macos_version_arm.to_float() < SENTRY_MACOS_MIN_VERSION);
}

String SentryEditorExportPluginMacOS::_get_export_option_warning(const Ref<EditorExportPlatform> &p_platform, const String &p_option) const {
	if ((is_builtin_option_or_hidden_export_option(p_option, "application/min_macos_version_x86_64") ||
				is_builtin_option_or_hidden_export_option(p_option, "application/min_macos_version_arm64")) &&
			_has_incompatible_minimum_version()) {
		return vformat("Sentry requires each exported architecture to target macOS %.1f or higher. Please adjust these settings.", SENTRY_MACOS_MIN_VERSION);
	}
	return String();
}

void SentryEditorExportPluginMacOS::_export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) {
	_is_macos = p_features.has("macos");
}

void SentryEditorExportPluginMacOS::_export_end() {
	if (!_is_macos) {
		return;
	}
	if (_has_incompatible_minimum_version()) {
		ERR_PRINT_ED(vformat("Sentry requires the minimum macOS version for each exported architecture to be %.1f or higher. The export completed, but the app may fail to launch on older macOS versions. Please update these export settings before distributing your app.", SENTRY_MACOS_MIN_VERSION));
	}
}

#endif // TOOLS_ENABLED
