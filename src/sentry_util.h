#ifndef SENTRY_UTIL_H
#define SENTRY_UTIL_H

#include "sentry_options.h"

#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <iostream>

using namespace godot;

class SentryUtil {
public:
	static String get_screen_orientation_string(int32_t p_screen = -1);

	template <typename... Args>
	static void print_debug(const Variant &p_arg1, const Args &...p_args) {
		if (SentryOptions::get_singleton()->is_debug_enabled()) {
			std::cout << "[sentry] DEBUG ";
			for (const Variant &variant : { p_arg1, Variant(p_args)... }) {
				std::cout << variant.stringify().utf8();
			}
			std::cout << std::endl;
		}
	}

	template <typename... Args>
	static void print_error(const Variant &p_arg1, const Args &...p_args) {
		if (SentryOptions::get_singleton()->is_debug_enabled()) {
			std::cerr << "[sentry] ERROR ";
			for (const Variant &variant : { p_arg1, Variant(p_args)... }) {
				std::cerr << variant.stringify().utf8();
			}
			std::cerr << std::endl;
		}
	}
};

#endif // SENTRY_UTIL_H
