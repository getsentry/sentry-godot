#pragma once

#include <godot_cpp/variant/string.hpp>

namespace sentry::util {

// Adds missing execute bits, which archive extraction may drop.
void ensure_unix_executable(const godot::String &p_path);

} // namespace sentry::util
