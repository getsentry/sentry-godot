#pragma once

#include "sentry/sentry_options.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace sentry::crash_reporter {

// Returns the appearance config for the crash reporter, holding only the keys that are set.
// The logo paths are absolute paths of the logo files, or empty if there are none.
godot::Dictionary make_app_config(const godot::Ref<SentryCrashReporterOptions> &p_options, const godot::String &p_logo_light_theme_path, const godot::String &p_logo_dark_theme_path);

// Writes p_config as "appsettings.json" into p_dir, where the reporter should look for it.
// Removes the file if p_config is empty.
void write_app_config(const godot::Dictionary &p_config, const godot::String &p_dir);

} // namespace sentry::crash_reporter
