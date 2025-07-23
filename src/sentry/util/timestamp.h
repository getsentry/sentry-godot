#ifndef SENTRY_TIMESTAMP_H
#define SENTRY_TIMESTAMP_H

#include <godot_cpp/variant/string.hpp>

namespace sentry::util {

bool is_valid_timestamp(const godot::String &p_timestamp);

// Convert a formatted RFC3339 timestamp string to microseconds since Unix epoch.
int64_t rfc3339_timestamp_to_microseconds(const char *p_formatted_cstring);

// Convert microseconds since Unix epoch to a formatted RFC3339 timestamp string.
godot::String microseconds_to_rfc3339_timestamp(int64_t p_microseconds_since_unix_epoch);

} //namespace sentry::util

#endif // SENTRY_TIMESTAMP_H
