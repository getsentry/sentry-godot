#pragma once

#include <godot_cpp/godot.hpp>
#include <godot_cpp/templates/span.hpp>
#include <godot_cpp/variant/string.hpp>
#include <string_view>

namespace sentry::util {

/// Copies UTF-32 text into an owning String, stopping at the span's end or the first NUL.
_FORCE_INLINE_ godot::String string_from_span(godot::Span<char32_t> p_text) {
	godot::String result;
	if (!p_text.is_empty()) {
		// The engine placement-constructs the String.
		godot::gdextension_interface::string_new_with_utf32_chars_and_len(result._native_ptr(), p_text.ptr(), p_text.size());
	}
	return result;
}

/// Returns true for ASCII letters (A-Z, a-z).
constexpr bool is_alpha_ascii(char32_t p_char) {
	return (p_char >= 'a' && p_char <= 'z') || (p_char >= 'A' && p_char <= 'Z');
}

/// Returns true for ASCII digits (0-9).
constexpr bool is_numeric_ascii(char32_t p_char) {
	return p_char >= '0' && p_char <= '9';
}

// Returns true if p_string ends with the given suffix, case-insensitive.
// Suffix must contain only lowercase ASCII characters.
bool ends_with_nocase_ascii(const godot::String &p_string, std::string_view p_lowercase_suffix);

} // namespace sentry::util
