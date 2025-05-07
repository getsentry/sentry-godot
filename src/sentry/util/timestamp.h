#ifndef SENTRY_TIMESTAMP_H
#define SENTRY_TIMESTAMP_H

#include <godot_cpp/variant/string.hpp>

namespace sentry::util {

bool is_valid_timestamp(const godot::String &p_timestamp);
}

#endif // SENTRY_TIMESTAMP_H
