#pragma once

#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::dotnet {

// Calls the .NET init, if available.
void init();

// Forwards a C# exception error to the .NET layer for capture.
void handle_logger_error(const String &p_file, const String &p_code);

} // namespace sentry::dotnet
