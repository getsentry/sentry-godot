#include "sentry_unit.h"

#include "sentry/util/simple_bind.h"

#include <godot_cpp/classes/engine.hpp>

namespace sentry {

SentryUnit *SentryUnit::singleton = nullptr;

void SentryUnit::create_singleton() {
	ERR_FAIL_NULL(Engine::get_singleton());
	singleton = memnew(SentryUnit);
	Engine::get_singleton()->register_singleton("SentryUnit", singleton);
}

void SentryUnit::destroy_singleton() {
	ERR_FAIL_NULL(Engine::get_singleton());
	if (!singleton) {
		return;
	}
	Engine::get_singleton()->unregister_singleton("SentryUnit");
	memdelete(singleton);
	singleton = nullptr;
}

void SentryUnit::_bind_methods() {
	// Duration
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "nanosecond"), get_nanosecond);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "microsecond"), get_microsecond);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "millisecond"), get_millisecond);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "second"), get_second);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "minute"), get_minute);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "hour"), get_hour);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "day"), get_day);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "week"), get_week);

	// Information
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "bit"), get_bit);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "byte"), get_byte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "kilobyte"), get_kilobyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "kibibyte"), get_kibibyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "megabyte"), get_megabyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "mebibyte"), get_mebibyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "gigabyte"), get_gigabyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "gibibyte"), get_gibibyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "terabyte"), get_terabyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "tebibyte"), get_tebibyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "petabyte"), get_petabyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "pebibyte"), get_pebibyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "exabyte"), get_exabyte);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "exbibyte"), get_exbibyte);

	// Fraction
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "ratio"), get_ratio);
	BIND_PROPERTY_READONLY(SentryUnit, PropertyInfo(Variant::STRING, "percent"), get_percent);
}

} // namespace sentry
