#pragma once

#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/string.hpp>

namespace sentry {

enum LogLevel {
	LOG_LEVEL_TRACE,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_FATAL
};

godot::String log_level_as_string(LogLevel p_level);
LogLevel log_level_from_string(const godot::String &p_value, LogLevel p_default);

inline godot::PropertyInfo make_log_level_enum_property(const godot::String &p_name) {
	return godot::PropertyInfo(godot::Variant::INT, p_name, godot::PROPERTY_HINT_ENUM, "Trace,Debug,Info,Warn,Error,Fatal");
}

} //namespace sentry
