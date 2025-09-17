#pragma once

#include <godot_cpp/variant/string.hpp>

namespace sentry::util {

class FastStringBuilder {
	godot::String buffer;
	char32_t *ptrw;
	size_t used = 0;
	size_t capacity;

private:
	_FORCE_INLINE_ void _resize(size_t p_size) {
		capacity = p_size;
		buffer.resize(capacity);
		ptrw = buffer.ptrw() + used;
	}

	_FORCE_INLINE_ void _ensure_capacity(size_t p_expected) {
		if (p_expected > capacity) {
			_resize(p_expected * 2);
		}
	}

public:
	FastStringBuilder(size_t p_estimate = 4096) {
		_resize(p_estimate);
	}

	void append(const char *p_cstr) {
		const size_t length = strlen(p_cstr);
		_ensure_capacity(used + length + 1);

		const char *src = p_cstr;
		const char *end = p_cstr + length;

		for (; src < end; ++src, ++ptrw) {
			*ptrw = static_cast<uint8_t>(*src);
		}

		used += length;
	}

	void append(const godot::String &p_str) {
		const size_t length = p_str.length();
		_ensure_capacity(used + length + 1);

		memcpy(ptrw, p_str.ptr(), length * sizeof(char32_t));
		ptrw += length;
		used += length;
	}

	bool ends_with(const char *p_suffix) {
		int suffix_len = strlen(p_suffix);
		if (suffix_len > used) {
			return false;
		}

		const char32_t *end_ptr = ptrw - suffix_len;
		for (int i = 0; i < suffix_len; ++i) {
			if (end_ptr[i] != static_cast<char32_t>(static_cast<uint8_t>(p_suffix[i]))) {
				return false;
			}
		}

		return true;
	}

	size_t get_length() {
		return used + 1;
	}

	godot::String to_string() {
		*ptrw = 0;
		_resize(used + 1);
		return buffer;
	}
};

} //namespace sentry::util
