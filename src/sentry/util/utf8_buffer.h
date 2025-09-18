#pragma once

#include <godot_cpp/variant/string.hpp>

namespace sentry::util {

// UTF-8 buffer designed for efficient large text accumulation.
// Minimal features with dynamic resizing and manual capacity management for performance.
// Best practice: generously over-estimate initial capacity to minimize reallocations.
class UTF8Buffer {
private:
	size_t capacity;
	char *start;
	char *write;

	void _ensure_capacity(size_t p_expected) {
		if (p_expected > capacity) {
			size_t new_capacity = capacity * 2;
			while (p_expected > new_capacity) {
				new_capacity = new_capacity * 2;
			}
			resize(new_capacity);
		}
	}

public:
	UTF8Buffer(size_t p_capacity = 4096) {
		if (p_capacity > 0) {
			start = (char *)memalloc(p_capacity);
			write = start;
			capacity = p_capacity;
		} else {
			p_capacity = 0;
			start = nullptr;
			write = nullptr;
		}
	}

	// Move constructor
	UTF8Buffer(UTF8Buffer &&p_other) :
			capacity(p_other.capacity), start(p_other.start), write(p_other.write) {
		p_other.capacity = 0;
		p_other.start = nullptr;
		p_other.write = nullptr;
	}

	~UTF8Buffer() {
		if (start) {
			free();
		}
	}

	void free() {
		memfree(start);
		start = nullptr;
		write = nullptr;
		capacity = 0;
	}

	const char *ptr() {
		return start;
	}

	size_t get_used() {
		return write - start;
	}

	size_t get_capacity() {
		return capacity;
	}

	void resize(size_t p_size) {
		capacity = p_size;
		size_t used = get_used();
		char *new_buffer = (char *)memalloc(capacity);
		memcpy(new_buffer, start, used);
		memfree(start);
		start = new_buffer;
		write = start + used;
	}

	void append(const char *p_cstr) {
		const size_t length = strlen(p_cstr);
		_ensure_capacity(get_used() + length + 1);

		const char *src = p_cstr;
		const char *end = p_cstr + length;

		for (; src < end; ++src, ++write) {
			*write = static_cast<uint8_t>(*src);
		}
	}

	void append(const godot::String &p_str) {
		const size_t length = p_str.length();
		_ensure_capacity(get_used() + length * 4 + 1); // ensure maximum theoretical

		// To UTF-8
		for (size_t i = 0; i < length; ++i) {
			char32_t c = p_str[i];
			if (c <= 0x7F) { // 1 byte
				*(write++) = static_cast<char>(c);
			} else if (c <= 0x7FF) { // 2 bytes
				*(write++) = static_cast<char>(0xC0 | ((c >> 6) & 0x1F)); // top 5 bits
				*(write++) = static_cast<char>(0x80 | (c & 0x3F)); // bottom 6 bits
			} else if (c <= 0xFFFF) { // 3 bytes
				*(write++) = static_cast<char>(0xE0 | ((c >> 12) & 0x0F)); // top 4 bits
				*(write++) = static_cast<char>(0x80 | ((c >> 6) & 0x3F)); // middle 6 bits
				*(write++) = static_cast<char>(0x80 | (c & 0x3F)); // bottom 6 bits
			} else { // 4 bytes
				*(write++) = static_cast<char>(0xF0 | ((c >> 18) & 0x07)); // top 3 bits
				*(write++) = static_cast<char>(0x80 | ((c >> 12) & 0x3F)); // upper middle 6 bits
				*(write++) = static_cast<char>(0x80 | ((c >> 6) & 0x3F)); // lower middle 6 bits
				*(write++) = static_cast<char>(0x80 | (c & 0x3F)); // bottom 6 bits
			}
		}
	}

	bool ends_with(const char *p_suffix) {
		int suffix_len = strlen(p_suffix);
		if (suffix_len > get_used()) {
			return false;
		}

		const char *end_ptr = write - suffix_len;
		for (int i = 0; i < suffix_len; ++i) {
			if (end_ptr[i] != p_suffix[i]) {
				return false;
			}
		}

		return true;
	}
};

} //namespace sentry::util
