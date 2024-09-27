#ifndef SENTRY_UTIL_H
#define SENTRY_UTIL_H

#include "sentry.h"

#include "godot_cpp/variant/variant.hpp"

class SentryUtil {
public:
	static sentry_value_t variant_to_sentry_value(const godot::Variant &p_variant);
};

#endif // SENTRY_UTIL_H
