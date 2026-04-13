#pragma once

#include <godot_cpp/variant/string.hpp>
#include <string_view>

namespace sentry::util {

// Returns true if p_string ends with the given suffix, case-insensitive.
// Suffix must contain only lowercase ASCII characters.
bool ends_with_nocase_ascii(const godot::String &p_string, std::string_view p_lowercase_suffix);

} // namespace sentry::util
