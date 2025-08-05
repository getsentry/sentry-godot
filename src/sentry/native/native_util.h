#ifndef NATIVE_UTIL_H
#define NATIVE_UTIL_H

#include "sentry/level.h"

#include <sentry.h>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry::native {

// Convert Godot Variant to sentry_value_t.
sentry_value_t variant_to_sentry_value(const Variant &p_variant, int p_depth = 0);

// Convert PackedStringArray to sentry_value_t (as a list).
sentry_value_t strings_to_sentry_list(const PackedStringArray &p_strings);

sentry_level_t level_to_native(Level p_level);
Level native_to_level(sentry_level_t p_native_level);

CharString level_to_cstring(Level p_level);
Level cstring_to_level(const CharString &p_cstring);

} //namespace sentry::native

#endif // NATIVE_UTIL_H
