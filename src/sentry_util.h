#ifndef SENTRY_UTIL_H
#define SENTRY_UTIL_H

#include "sentry_options.h"

#include <sentry.h>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <iostream>

using namespace godot;

class SentryUtil {
public:
	static sentry_value_t variant_to_sentry_value(const Variant &p_variant);
	static String get_screen_orientation_string(int32_t p_screen = -1);
	static void sentry_event_set_context(sentry_value_t p_event, const char *p_context_name, sentry_value_t p_context);
	static godot::CharString generate_uuid();
	static sentry_value_t strings_to_sentry_list(const PackedStringArray &p_strings);

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
