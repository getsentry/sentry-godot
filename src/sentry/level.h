#ifndef SENTRY_LEVEL_H
#define SENTRY_LEVEL_H

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/string.hpp>

namespace sentry {

// Represents the severity of events or breadcrumbs.
// In the public API, it is exposed as SentrySDK.Level enum.
// And as such, VariantCaster<SentrySDK::Level> is defined in sentry_sdk.h.
enum Level {
	LEVEL_DEBUG = 0,
	LEVEL_INFO = 1,
	LEVEL_WARNING = 2,
	LEVEL_ERROR = 3,
	LEVEL_FATAL = 4
};

godot::CharString level_as_cstring(Level level);
godot::PropertyInfo make_level_enum_property(const godot::String &p_name);
Level int_to_level(int p_value);

} // namespace sentry

#endif // SENTRY_LEVEL_H
