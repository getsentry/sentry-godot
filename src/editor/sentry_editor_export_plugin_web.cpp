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

void SentryEditorExportPluginWeb::_export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) {
	ERR_FAIL_NULL(ProjectSettings::get_singleton());

	String export_dir = ProjectSettings::get_singleton()->globalize_path(p_path.get_base_dir());

	sentry::logging::print_debug("Exporting project to ", export_dir);

	// Add HTML head content.

	ERR_FAIL_COND(get_export_preset().is_null());

	String head_include = get_export_preset()->get("html/head_include");
	bool adding = false;

	for (const String &line : html_head_content) {
		if (!head_include.contains(line)) {
			if (!adding) {
				adding = true;
				head_include += "\n" + html_head_comment;
			}
			head_include += "\n" + line;
		}
	}

	get_export_preset()->set("html/head_include", head_include);

	// Copy JS files.

	Ref<DirAccess> dir = DirAccess::open(project_js_files_path);
	ERR_FAIL_COND_MSG(dir.is_null(), vformat("Sentry: Failed to open directory \"%s\" on export to Web.", project_js_files_path));

	Error err = dir->list_dir_begin();
	ERR_FAIL_COND_MSG(err != OK, vformat("Sentry: Failed to list \"%s\" contents on export to Web: %s", project_js_files_path, UtilityFunctions::error_string(err)));

	String fn = dir->get_next();
	while (!fn.is_empty()) {
		if (!dir->current_is_dir() && (fn.ends_with(".js") || fn.ends_with(".map"))) {
			String src_abs = ProjectSettings::get_singleton()->globalize_path("res://addons/sentry/web/").path_join(fn);
			String dest_abs = export_dir.path_join(fn);

			Error err = DirAccess::copy_absolute(src_abs, dest_abs);
			if (err == OK) {
				sentry::logging::print_debug("  Copied ", fn);
			} else {
				ERR_PRINT(vformat("Sentry: Failed to copy \"%s\" on export to Web: %s", fn, UtilityFunctions::error_string(err)));
			}
		}

		fn = dir->get_next();
	}

	dir->list_dir_end();
}

SentryEditorExportPluginWeb::SentryEditorExportPluginWeb() {
	project_js_files_path = "res://addons/sentry/web/";

	html_head_comment = "<!-- Automatically added by Sentry SDK -->";

	html_head_content.append("<script src=\"bundle.debug.min.js\" crossorigin=\"anonymous\"></script>");
	html_head_content.append("<script src=\"wasm.debug.min.js\" crossorigin=\"anonymous\"></script>");
	html_head_content.append("<script src=\"sentry-bridge.js\" crossorigin=\"anonymous\"></script>");
}

#endif // TOOLS_ENABLED
