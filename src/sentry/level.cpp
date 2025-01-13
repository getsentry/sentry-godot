#include "level.h"

namespace sentry {

godot::CharString level_as_cstring(Level level) {
	switch (level) {
		case Level::LEVEL_DEBUG:
			return "debug";
		case Level::LEVEL_INFO:
			return "info";
		case Level::LEVEL_WARNING:
			return "warning";
		case Level::LEVEL_ERROR:
			return "error";
		case Level::LEVEL_FATAL:
			return "fatal";
		default:
			return "unknown";
	}
}

godot::PropertyInfo make_level_enum_property(const godot::String &p_name) {
	return godot::PropertyInfo(godot::Variant::INT, p_name, godot::PROPERTY_HINT_ENUM, "Debug,Info,Warning,Error,Fatal");
}

} // namespace sentry
