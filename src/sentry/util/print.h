#ifndef SENTRY_PRINT_H
#define SENTRY_PRINT_H

#include "sentry/level.h"
#include "sentry_options.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry::util {

template <typename... Args>
void print(sentry::Level p_level, const Variant &p_arg1, const Args &...p_args) {
	if (!SentryOptions::get_singleton()->is_debug_enabled() && p_level < sentry::LEVEL_ERROR) {
		return;
	}
	if (SentryOptions::get_singleton()->get_diagnostic_level() > p_level) {
		return;
	}

	switch (p_level) {
		case LEVEL_DEBUG:
			UtilityFunctions::print("Sentry: DEBUG: ", p_arg1, p_args...);
			break;
		case LEVEL_INFO:
			UtilityFunctions::print("Sentry: INFO: ", p_arg1, p_args...);
			break;
		case LEVEL_WARNING:
			UtilityFunctions::printerr("Sentry: WARNING: ", p_arg1, p_args...);
			break;
		case LEVEL_ERROR:
			UtilityFunctions::printerr("Sentry: ERROR: ", p_arg1, p_args...);
			break;
		case LEVEL_FATAL:
			UtilityFunctions::printerr("Sentry: FATAL: ", p_arg1, p_args...);
			break;
	}
}

template <typename... Args>
void print_debug(const Variant &p_arg1, const Args &...p_args) {
	print(Level::LEVEL_DEBUG, p_arg1, p_args...);
}

template <typename... Args>
void print_info(const Variant &p_arg1, const Args &...p_args) {
	print(Level::LEVEL_INFO, p_arg1, p_args...);
}

template <typename... Args>
void print_warning(const Variant &p_arg1, const Args &...p_args) {
	print(Level::LEVEL_WARNING, p_arg1, p_args...);
}

template <typename... Args>
void print_error(const Variant &p_arg1, const Args &...p_args) {
	print(Level::LEVEL_ERROR, p_arg1, p_args...);
}

template <typename... Args>
void print_fatal(const Variant &p_arg1, const Args &...p_args) {
	print(Level::LEVEL_FATAL, p_arg1, p_args...);
}

} //namespace sentry::util

#endif // SENTRY_PRINT_H
