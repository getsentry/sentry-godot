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

bool SentryEditorExportPluginMacOS::_is_minimum_version_incompatible(const String &p_architecture, const String &p_option) const {
	String architecture = get_option("binary_format/architecture");
	String minimum_version = get_option(p_option);
	return (architecture == "universal" || architecture == p_architecture) && minimum_version.to_float() < SENTRY_MACOS_MIN_VERSION;
}

bool SentryEditorExportPluginMacOS::_has_incompatible_minimum_version() const {
	return _is_minimum_version_incompatible("x86_64", "application/min_macos_version_x86_64") ||
			_is_minimum_version_incompatible("arm64", "application/min_macos_version_arm64");
}

String SentryEditorExportPluginMacOS::_get_export_option_warning(const Ref<EditorExportPlatform> &p_platform, const String &p_option) const {
	bool incompatible = false;
	if (p_option == "application/min_macos_version_x86_64") {
		incompatible = _is_minimum_version_incompatible("x86_64", p_option);
	} else if (p_option == "application/min_macos_version_arm64") {
		incompatible = _is_minimum_version_incompatible("arm64", p_option);
	} else if (p_option == "sentry/_export_check") {
		incompatible = _has_incompatible_minimum_version();
	}
	if (incompatible) {
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
