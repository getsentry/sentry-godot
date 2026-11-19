#include "file_permissions.h"

#include "sentry/logging/print.h"

#include <godot_cpp/classes/file_access.hpp>

using namespace godot;

namespace sentry::util {

void ensure_unix_executable(const String &p_path) {
	if (!FileAccess::file_exists(p_path)) {
		return;
	}

	BitField<FileAccess::UnixPermissionFlags> perm = FileAccess::get_unix_permissions(p_path);
	BitField<FileAccess::UnixPermissionFlags> new_perm = perm;

	if (!perm.has_flag(FileAccess::UNIX_EXECUTE_OWNER)) {
		new_perm.set_flag(FileAccess::UNIX_EXECUTE_OWNER);
	}
	if (!perm.has_flag(FileAccess::UNIX_EXECUTE_GROUP)) {
		new_perm.set_flag(FileAccess::UNIX_EXECUTE_GROUP);
	}
	if (!perm.has_flag(FileAccess::UNIX_EXECUTE_OTHER)) {
		new_perm.set_flag(FileAccess::UNIX_EXECUTE_OTHER);
	}

	if (perm != new_perm) {
		godot::Error err = FileAccess::set_unix_permissions(p_path, new_perm);
		if (err != OK && err != ERR_UNAVAILABLE) {
			sentry::logging::print_error(vformat("Failed to set executable permissions for %s (error code %d)", p_path, err));
		}
	}
}

} // namespace sentry::util
