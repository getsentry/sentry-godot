#ifndef NATIVE_UTIL_H
#define NATIVE_UTIL_H

#include <sentry.h>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry::native {

// Convert Godot Variant to sentry_value_t.
sentry_value_t variant_to_sentry_value(const Variant &p_variant);

// Convert PackedStringArray to sentry_value_t (as a list).
sentry_value_t strings_to_sentry_list(const PackedStringArray &p_strings);

} //namespace sentry::native

#endif // NATIVE_UTIL_H
