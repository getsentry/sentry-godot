#include "project_settings.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

namespace sentry::util {

String get_engine_log_path() {
	String log_path = ProjectSettings::get_singleton()->get_setting("debug/file_logging/log_path");
	ERR_FAIL_COND_V_MSG(!FileAccess::file_exists(log_path), String(), "Sentry: Log file not found. Make sure \"debug/file_logging/enable_file_logging\" is turned ON in the Project Settings.");
	return log_path.replace("user://", OS::get_singleton()->get_user_data_dir() + "/");
}

} // namespace sentry::util
