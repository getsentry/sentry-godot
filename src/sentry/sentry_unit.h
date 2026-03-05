#pragma once

#include <godot_cpp/core/object.hpp>

using namespace godot;

namespace sentry {

// Unit constants for metrics and attributes
// https://develop.sentry.dev/sdk/foundations/state-management/scopes/attributes/#units
class SentryUnit : public Object {
	GDCLASS(SentryUnit, Object);

	static SentryUnit *singleton;

protected:
	static void _bind_methods();

public:
	// Duration
	String get_nanosecond() const { return "nanosecond"; }
	String get_microsecond() const { return "microsecond"; }
	String get_millisecond() const { return "millisecond"; }
	String get_second() const { return "second"; }
	String get_minute() const { return "minute"; }
	String get_hour() const { return "hour"; }
	String get_day() const { return "day"; }
	String get_week() const { return "week"; }

	// Information
	String get_bit() const { return "bit"; }
	String get_byte() const { return "byte"; }
	String get_kilobyte() const { return "kilobyte"; }
	String get_kibibyte() const { return "kibibyte"; }
	String get_megabyte() const { return "megabyte"; }
	String get_mebibyte() const { return "mebibyte"; }
	String get_gigabyte() const { return "gigabyte"; }
	String get_gibibyte() const { return "gibibyte"; }
	String get_terabyte() const { return "terabyte"; }
	String get_tebibyte() const { return "tebibyte"; }
	String get_petabyte() const { return "petabyte"; }
	String get_pebibyte() const { return "pebibyte"; }
	String get_exabyte() const { return "exabyte"; }
	String get_exbibyte() const { return "exbibyte"; }

	// Fraction
	String get_ratio() const { return "ratio"; }
	String get_percent() const { return "percent"; }

	static SentryUnit *get_singleton() { return singleton; }
	static void create_singleton();
	static void destroy_singleton();

	SentryUnit() = default;
};

} // namespace sentry
