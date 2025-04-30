#include "level.h"

#include "sentry/util/print.h"

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

Level int_to_level(int p_value) {
	switch (p_value) {
		case 0:
			return Level::LEVEL_DEBUG;
		case 1:
			return Level::LEVEL_INFO;
		case 2:
			return Level::LEVEL_WARNING;
		case 3:
			return Level::LEVEL_ERROR;
		case 4:
			return Level::LEVEL_FATAL;
		default:
			sentry::util::print_error("Internal Error -- unexpected level integer: " + godot::String::num_int64(p_value));
			return Level::LEVEL_ERROR;
	}
}

} // namespace sentry
