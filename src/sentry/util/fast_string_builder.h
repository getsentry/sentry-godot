#pragma once

#include <godot_cpp/variant/string.hpp>

namespace sentry::util {

class FastStringBuilder {
	godot::String buffer;
	char32_t *ptrw = nullptr;
	size_t sz = 0;
	size_t max_sz = 0;

private:
	inline void _ensure_size(size_t p_expected) {
		if (p_expected > max_sz) {
			max_sz = max_sz * 2;
			buffer.resize(max_sz);
			ptrw = buffer.ptrw();
		}
	}

public:
	FastStringBuilder(size_t p_estimate = 4096) {
		max_sz = p_estimate;
		buffer.resize(max_sz);
		ptrw = buffer.ptrw();
	}

	void append(const char *p_cstr) {
		const size_t length = strlen(p_cstr);
		_ensure_size(sz + length + 1);

		const char *src = p_cstr;
		const char *end = p_cstr + length;

		for (; src < end; ++src, ++ptrw) {
			*ptrw = static_cast<uint8_t>(*src);
		}

		sz += length;
	}

	void append(const godot::String &p_str) {
		const size_t length = p_str.length();
		_ensure_size(sz + length + 1);

		memcpy(ptrw, p_str.ptr(), length * sizeof(char32_t));
		ptrw += length;
		sz += length;
	}

	bool ends_with(const char *p_suffix) {
		if (sz == 0) {
			return false;
		}

		int suffix_len = strlen(p_suffix);
		if (suffix_len > sz) {
			return false;
		}

		// Compare from the end backwards
		const char32_t *end_ptr = ptrw - suffix_len;
		for (int i = 0; i < suffix_len; ++i) {
			if (end_ptr[i] != static_cast<char32_t>(static_cast<uint8_t>(p_suffix[i]))) {
				return false;
			}
		}

		return true;
	}

	size_t get_length() {
		return sz + 1;
	}

	godot::String to_string() {
		*ptrw = 0;
		max_sz = sz + 1;
		buffer.resize(max_sz);
		ptrw = buffer.ptrw();
		return buffer;
	}
};

} //namespace sentry::util
