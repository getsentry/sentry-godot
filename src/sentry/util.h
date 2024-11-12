#ifndef UTIL_H
#define UTIL_H

#include "sentry_options.h"

#include <godot_cpp/variant/variant.hpp>
#include <iostream>

using namespace godot;

namespace sentry::util {

template <typename... Args>
void print_debug(const Variant &p_arg1, const Args &...p_args) {
	if (SentryOptions::get_singleton()->is_debug_enabled()) {
		std::cout << "[sentry] DEBUG ";
		for (const Variant &variant : { p_arg1, Variant(p_args)... }) {
			std::cout << variant.stringify().utf8();
		}
		std::cout << std::endl;
	}
}

template <typename... Args>
void print_error(const Variant &p_arg1, const Args &...p_args) {
	if (SentryOptions::get_singleton()->is_debug_enabled()) {
		std::cerr << "[sentry] ERROR ";
		for (const Variant &variant : { p_arg1, Variant(p_args)... }) {
			std::cerr << variant.stringify().utf8();
		}
		std::cerr << std::endl;
	}
}

} //namespace sentry::util

#endif // UTIL_H
