#include "log_level.h"

namespace sentry {

godot::String log_level_as_string(LogLevel p_level) {
	switch (p_level) {
		case LOG_LEVEL_TRACE:
			return "trace";
		case LOG_LEVEL_DEBUG:
			return "debug";
		case LOG_LEVEL_INFO:
			return "info";
		case LOG_LEVEL_WARN:
			return "warn";
		case LOG_LEVEL_ERROR:
			return "error";
		case LOG_LEVEL_FATAL:
			return "fatal";
		default:
			return "unknown";
	}
}

LogLevel log_level_from_string(const godot::String &p_value, LogLevel p_default) {
	if (p_value == "trace") {
		return LOG_LEVEL_TRACE;
	} else if (p_value == "debug") {
		return LOG_LEVEL_DEBUG;
	} else if (p_value == "info") {
		return LOG_LEVEL_INFO;
	} else if (p_value == "warn") {
		return LOG_LEVEL_WARN;
	} else if (p_value == "error") {
		return LOG_LEVEL_ERROR;
	} else if (p_value == "fatal") {
		return LOG_LEVEL_FATAL;
	} else {
		return p_default;
	}
}

} // namespace sentry
