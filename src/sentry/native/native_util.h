#pragma once

#include "godot_cpp/core/defs.hpp"
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

// TODO: move this to level.h
CharString level_to_cstring(Level p_level);
Level cstring_to_level(const CharString &p_cstring);

_FORCE_INLINE_ void sentry_value_set_or_remove_string_by_key(sentry_value_t value, const char *k, const String &v) {
	if (v.is_empty()) {
		sentry_value_remove_by_key(value, k);
	} else {
		sentry_value_set_by_key(value, k, sentry_value_new_string(v.utf8()));
	}
}

sentry_value_t variant_to_attribute(const Variant &p_value);
sentry_value_t dictionary_to_attributes(const Dictionary &p_attributes);

Variant sentry_value_get_attribute(sentry_value_t p_value, const String &p_name);
void sentry_value_set_attribute(sentry_value_t p_native, const String &p_name, const Variant &p_value);
void sentry_value_add_attributes(sentry_value_t p_native, const Dictionary &p_attributes);

} //namespace sentry::native
