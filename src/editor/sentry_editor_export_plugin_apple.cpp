#include "sentry_editor_export_plugin_apple.h"

#ifdef TOOLS_ENABLED

#include "editor/sentry_editor_export_plugin_apple.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/string.hpp>

namespace {

void _copy_dir_preserving_links(const String &p_source, const String &p_dest) {
	Ref<DirAccess> dest_dir = DirAccess::open(".");
	if (!dest_dir->dir_exists(p_dest)) {
		Error err = dest_dir->make_dir_recursive(p_dest);
		ERR_FAIL_COND_MSG(err != OK, "Failed to create destination directory: " + p_dest + " Error: " + itos(err));
	}

	dest_dir = DirAccess::open(p_dest);
	ERR_FAIL_COND_MSG(dest_dir.is_null(), "Failed to open destination directory: " + p_dest);

	Ref<DirAccess> source_dir = DirAccess::open(p_source);
	ERR_FAIL_COND_MSG(source_dir.is_null(), "Failed to open source directory: " + p_source);

	source_dir->list_dir_begin();
	String item = source_dir->get_next();
	while (!item.is_empty()) {
		if (item == "." || item == "..") {
			item = source_dir->get_next();
			continue;
		}

		String source_path = p_source.path_join(item);
		String dest_path = p_dest.path_join(item);
		if (source_dir->is_link(item)) {
			String link_target = source_dir->read_link(item);
			Error err = dest_dir->create_link(link_target, item);
			ERR_FAIL_COND_MSG(err != OK, "Failed to create symbolic link: " + item + " -> " + link_target + " Error: " + itos(err));
		} else if (source_dir->current_is_dir()) {
			_copy_dir_preserving_links(source_path, dest_path);
		} else {
			Error err = dest_dir->copy(source_path, dest_path);
			ERR_FAIL_COND_MSG(err != OK, "Failed to copy file: " + source_path + " to " + dest_path + " Error: " + itos(err));
		}
		item = source_dir->get_next();
	}

	source_dir->list_dir_end();
}

} // unnamed namespace

String SentryEditorExportPluginApple::_get_name() const {
	return "SentryEditorExportPluginUnix";
}

bool SentryEditorExportPluginApple::_supports_platform(const Ref<EditorExportPlatform> &p_platform) const {
	return p_platform->get_os_name() == "macOS";
}

void SentryEditorExportPluginApple::_export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) {
	export_path = p_path;
}

void SentryEditorExportPluginApple::_export_end() {
	String source_path = "res://addons/sentry/bin/macos/Sentry.framework/";
	String target_path = export_path.path_join("Contents/Frameworks/Sentry.framework");
	print_verbose("Copying ", source_path, " -> ", target_path);
	_copy_dir_preserving_links(source_path, target_path);
}

#endif // TOOLS_ENABLED
