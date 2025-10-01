#pragma once

#include <godot_cpp/variant/variant.hpp>

namespace sentry::android {

godot::Variant sanitize_variant(const godot::Variant &p_value, int p_depth = 0);

} // namespace sentry::android
