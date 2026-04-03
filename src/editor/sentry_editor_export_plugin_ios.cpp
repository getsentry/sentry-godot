#include "sentry_editor_export_plugin_ios.h"

#ifdef TOOLS_ENABLED

#include "export_utils.h"
#include "gen/sdk_version.gen.h"
#include "sentry/logging/print.h"

using namespace sentry::editor;

String SentryEditorExportPluginIOS::_get_name() const {
	return "SentryEditorExportPluginIOS";
}

bool SentryEditorExportPluginIOS::_supports_platform(const Ref<EditorExportPlatform> &p_platform) const {
	return p_platform->get_os_name() == "iOS";
}

TypedArray<Dictionary> SentryEditorExportPluginIOS::_get_export_options(const Ref<EditorExportPlatform> &p_platform) const {
	TypedArray<Dictionary> options;
	// HACK: Adding a hidden option so we can show a warning at the bottom of the export dialog.
	options.push_back(make_hidden_export_check_option());
	return options;
}

String SentryEditorExportPluginIOS::_get_export_option_warning(const Ref<EditorExportPlatform> &p_platform, const String &p_option) const {
	// HACK: Godot shows bottom-bar warnings through a separate code path that only checks plugin-owned options.
	if (is_builtin_option_or_hidden_export_option(p_option, "application/min_ios_version")) {
		String min_ios_version = get_option("application/min_ios_version");
		if (min_ios_version.to_float() < SENTRY_IOS_MIN_VERSION) {
			return vformat("Sentry requires \"Min iOS version\" %.1f or higher. Please adjust this setting.", SENTRY_IOS_MIN_VERSION);
		}
	}
	return String();
}

void SentryEditorExportPluginIOS::_export_end() {
	String min_ios_version = get_option("application/min_ios_version");
	if (min_ios_version.to_float() < SENTRY_IOS_MIN_VERSION) {
		ERR_PRINT_ED(vformat("Sentry requires \"Min iOS version\" to be %.1f or higher. The export completed, but the app may fail App Store Connect validation. Please update this export setting before distributing your app.", SENTRY_IOS_MIN_VERSION));
	}
}

#endif // TOOLS_ENABLED
