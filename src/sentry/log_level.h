#pragma once

#include <godot_cpp/core/property_info.hpp>

namespace sentry {

enum LogLevel {
	LOG_LEVEL_TRACE,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_FATAL
};

inline godot::PropertyInfo make_log_level_enum_property(const godot::String &p_name) {
	return godot::PropertyInfo(godot::Variant::INT, p_name, godot::PROPERTY_HINT_ENUM, "Trace,Debug,Info,Warn,Error,Fatal");
}

} //namespace sentry
