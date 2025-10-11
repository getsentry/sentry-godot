#include "sentry_editor_export_plugin_unix.h"

#if defined(TOOLS_ENABLED) && !defined(WINDOWS_ENABLED)

#include "sentry/logging/print.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>

namespace {

void _set_executable_permissions(const String &p_path) {
	if (!FileAccess::file_exists(p_path)) {
		return;
	}

	BitField<FileAccess::UnixPermissionFlags> perm = FileAccess::get_unix_permissions(p_path);

	// Permissions for executable files should be set to 755.
	BitField<FileAccess::UnixPermissionFlags> new_perm =
			FileAccess::UNIX_READ_OWNER | FileAccess::UNIX_WRITE_OWNER | FileAccess::UNIX_EXECUTE_OWNER |
			FileAccess::UNIX_READ_GROUP | FileAccess::UNIX_EXECUTE_GROUP |
			FileAccess::UNIX_READ_OTHER | FileAccess::UNIX_EXECUTE_OTHER;

	if (perm != new_perm) {
		Error err = FileAccess::set_unix_permissions(p_path, new_perm);
		if (err != OK && err != ERR_UNAVAILABLE) {
			sentry::logging::print_warning("Failed to set executable permissions for: " + p_path);
		}
	}
}

void _find_and_fix_crashpad_handler(const String &p_path) {
	List<String> dirs_to_process;
	dirs_to_process.push_back(FileAccess::file_exists(p_path) ? p_path.get_base_dir() : p_path);

	while (!dirs_to_process.is_empty()) {
		String work_dir = dirs_to_process.back()->get();
		dirs_to_process.pop_back();

		Ref<DirAccess> da = DirAccess::open(work_dir);
		if (da.is_null()) {
			continue;
		}

		da->list_dir_begin();

		String file = da->get_next();
		while (!file.is_empty()) {
			String full_path = work_dir.path_join(file);

			if (da->current_is_dir() && file != "." && file != "..") {
				// Add subdirectory to be processed
				dirs_to_process.push_back(full_path);
			} else if (file == "crashpad_handler") {
				_set_executable_permissions(full_path);
			}

			file = da->get_next();
		}
		da->list_dir_end();
	}
}

} // unnamed namespace

String SentryEditorExportPluginUnix::_get_name() const {
	return "SentryEditorExportPluginUnix";
}

bool SentryEditorExportPluginUnix::_supports_platform(const Ref<EditorExportPlatform> &p_platform) const {
	return p_platform->get_os_name() != "Windows";
}

void SentryEditorExportPluginUnix::_export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) {
	export_path = p_path;
}

void SentryEditorExportPluginUnix::_export_end() {
	// Fix crashpad handler executable permissions on Unix platforms.
	_find_and_fix_crashpad_handler(export_path);
}

#endif // TOOLS_ENABLED && !WINDOWS_ENABLED
