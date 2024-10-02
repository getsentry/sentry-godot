#ifndef SENTRY_UTIL_H
#define SENTRY_UTIL_H

#include <sentry.h>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/variant.hpp>

class SentryUtil {
public:
	static sentry_value_t variant_to_sentry_value(const godot::Variant &p_variant);
	static godot::CharString get_screen_orientation_cstring(int32_t p_screen = -1);
};

#endif // SENTRY_UTIL_H
