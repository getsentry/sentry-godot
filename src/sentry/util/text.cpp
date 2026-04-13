#include "text.h"

using namespace godot;

namespace {

#ifdef DEBUG_ENABLED
bool _has_no_uppercase_ascii(std::string_view p_string) {
	for (char c : p_string) {
		if (c >= 'A' && c <= 'Z') {
			return false;
		}
	}
	return true;
}
#endif

} // unnamed namespace

namespace sentry::util {

bool ends_with_nocase_ascii(const String &p_text, std::string_view p_lowercase_suffix) {
#ifdef DEBUG_ENABLED
	DEV_ASSERT(_has_no_uppercase_ascii(p_lowercase_suffix));
#endif

	const int64_t suffix_len = static_cast<int64_t>(p_lowercase_suffix.size());
	const int64_t text_len = p_text.length();
	if (text_len < suffix_len) {
		return false;
	}

	const char32_t *ptr = p_text.ptr() + (text_len - suffix_len);

	for (int64_t i = 0; i < suffix_len; ++i) {
		char32_t c = ptr[i];
		if (c >= U'A' && c <= U'Z') {
			c += 32;
		}
		if (c != static_cast<char32_t>(p_lowercase_suffix[i])) {
			return false;
		}
	}

	return true;
}

} //namespace sentry::util
