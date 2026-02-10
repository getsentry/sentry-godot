#ifndef SENTRY_TIMESTAMP_H
#define SENTRY_TIMESTAMP_H

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

class SentryTimestamp : public RefCounted {
	GDCLASS(SentryTimestamp, RefCounted);

private:
	// NOTE: Use int64_t for Godot Variant compatibility.
	int64_t microseconds_since_unix_epoch = 0; // 1970-01-01

protected:
	static void _bind_methods();

	String _to_string() const { return to_rfc3339(); }

public:
	// Parse RFC3339 timestamp (YYYY-MM-DDTHH:MM:SS.sssssssssZ or with ±HH:MM offset).
	static Ref<SentryTimestamp> parse_rfc3339_cstr(const char *p_formatted_cstring);
	static Ref<SentryTimestamp> parse_rfc3339(const String &p_formatted_string) { return parse_rfc3339_cstr(p_formatted_string.ascii()); }

	// Create with Unix time (seconds since unix epoch) – useful in GDScript.
	static Ref<SentryTimestamp> from_unix_time(double p_unix_time);

	// Create with microseconds since Unix epoch – lossless.
	static Ref<SentryTimestamp> from_microseconds_since_unix_epoch(int64_t p_microseconds);

	_FORCE_INLINE_ int64_t get_microseconds_since_unix_epoch() const { return microseconds_since_unix_epoch; }
	_FORCE_INLINE_ void set_microseconds_since_unix_epoch(int64_t p_microseconds) { microseconds_since_unix_epoch = p_microseconds; }

	_FORCE_INLINE_ bool equals(const Ref<SentryTimestamp> &p_other) {
		return p_other.is_valid() ? microseconds_since_unix_epoch == p_other->microseconds_since_unix_epoch : false;
	}

	// Return RFC3339 formatted string.
	String to_rfc3339() const;

	// Return seconds since Unix epoch as double with microsecond precision.
	_FORCE_INLINE_ double to_unix_time() const { return microseconds_since_unix_epoch * 0.000'001; }
};

} // namespace sentry

#endif // SENTRY_TIMESTAMP_H
