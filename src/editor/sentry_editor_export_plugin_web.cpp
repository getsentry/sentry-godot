#include "sentry_editor_export_plugin_web.h"

#ifdef TOOLS_ENABLED

#include "sentry/logging/print.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>

String SentryEditorExportPluginWeb::_get_name() const {
	return "SentryEditorExportPluginWeb";
}

bool SentryEditorExportPluginWeb::_supports_platform(const Ref<EditorExportPlatform> &p_platform) const {
	return p_platform->get_os_name() == "Web";
}

TypedArray<Dictionary> SentryEditorExportPluginWeb::_get_export_options(const Ref<EditorExportPlatform> &p_platform) const {
	TypedArray<Dictionary> options;
	Dictionary option;
	option["option"] = Dictionary(PropertyInfo(Variant::BOOL, "sentry/inject_script"));
	option["default_value"] = true;
	options.push_back(option);
	return options;
}

void SentryEditorExportPluginWeb::_export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) {
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	String export_dir = ProjectSettings::get_singleton()->globalize_path(p_path.get_base_dir());

	sentry::logging::print_debug("Exporting project to ", export_dir);

	ERR_FAIL_COND(get_export_preset().is_null());

	// Add HTML head content.

	if (get_option("sentry/inject_script")) {
		String head_include = get_export_preset()->get("html/head_include");
		bool edited = false;
		for (const String &line : html_head_content) {
			if (!head_include.contains(line)) {
				if (!edited) {
					edited = true;
					head_include += "\n" + html_head_comment;
				}
				head_include += "\n" + line;
			}
		}
		if (edited) {
			get_export_preset()->set("html/head_include", head_include);
		}
	}

	// Copy JS files.

	Ref<DirAccess> dir = DirAccess::open(project_js_files_path);
	ERR_FAIL_COND_MSG(dir.is_null(), vformat("Sentry: Failed to open directory \"%s\" on export to Web.", project_js_files_path));

	Error err = dir->list_dir_begin();
	ERR_FAIL_COND_MSG(err != OK, vformat("Sentry: Failed to list \"%s\" contents on export to Web: %s", project_js_files_path, UtilityFunctions::error_string(err)));

	String fn = dir->get_next();
	while (!fn.is_empty()) {
		if (!dir->current_is_dir() && (fn.ends_with(".js") || fn.ends_with(".map"))) {
			String src_abs = ProjectSettings::get_singleton()->globalize_path(project_js_files_path).path_join(fn);
			String dest_abs = export_dir.path_join(fn);

			Error copy_err = DirAccess::copy_absolute(src_abs, dest_abs);
			if (copy_err == OK) {
				sentry::logging::print_debug("  Copied ", fn);
			} else {
				ERR_PRINT(vformat("Sentry: Failed to copy \"%s\" on export to Web: %s", fn, UtilityFunctions::error_string(copy_err)));
			}
		}

		fn = dir->get_next();
	}

	dir->list_dir_end();
}

String SentryEditorExportPluginWeb::_get_export_option_warning(const Ref<EditorExportPlatform> &p_platform, const String &p_option) const {
	// HACK: Also check "sentry/inject_script" so the warning appears at the bottom of the export dialog.
	//       Godot shows bottom-bar warnings through a separate code path that only checks plugin-owned options.
	if ((p_option == "variant/extensions_support" || p_option == "sentry/inject_script") &&
			get_option("variant/extensions_support") == Variant(false)) {
		return "Sentry requires \"Extension Support\" to be enabled for Web exports.";
	}
	return String();
}

SentryEditorExportPluginWeb::SentryEditorExportPluginWeb() {
	project_js_files_path = "res://addons/sentry/web/";

	html_head_comment = "<!-- Automatically added by Sentry SDK -->";

	html_head_content.append("<script src=\"sentry-bundle.js\" crossorigin=\"anonymous\"></script>");
}

#endif // TOOLS_ENABLED
