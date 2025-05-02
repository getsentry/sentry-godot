#ifndef SENTRY_UTIL_PROJECT_SETTINGS_H
#define SENTRY_UTIL_PROJECT_SETTINGS_H

#include <godot_cpp/variant/string.hpp>

namespace sentry::util {

// Returns log file path or an empty string if it doesn't exist.
godot::String get_engine_log_path();

} //namespace sentry::util

#endif // SENTRY_UTIL_PROJECT_SETTINGS_H
