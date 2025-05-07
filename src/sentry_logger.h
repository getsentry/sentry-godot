#ifndef SENTRY_LOGGER_H
#define SENTRY_LOGGER_H

#include "sentry/godot_error_types.h"

#include <godot_cpp/classes/logger.hpp>
#include <godot_cpp/classes/script_backtrace.hpp>

using namespace godot;

class SentryLogger : public Logger {
	GDCLASS(SentryLogger, Logger);

private:
	using GodotErrorType = sentry::GodotErrorType;
	using SourceLine = std::pair<std::string, int>;
	using TimePoint = std::chrono::high_resolution_clock::time_point;

	struct SourceLineHash {
		std::size_t operator()(const SourceLine &p_source_line) const {
			return std::hash<std::string>()(p_source_line.first) ^ std::hash<int>()(p_source_line.second);
		}
	};

	// Stores the last time an error was logged for each source line that generated an error.
	std::unordered_map<SourceLine, TimePoint, SourceLineHash> source_line_times;

	// Time points for events captured within throttling window.
	std::deque<TimePoint> event_times;

protected:
	static void _bind_methods() {}

public:
	virtual void _log_error(const String &p_function, const String &p_file, int32_t p_line, const String &p_code, const String &p_rationale, bool p_editor_notify, int32_t p_error_type, const TypedArray<ScriptBacktrace> &p_script_backtraces) override;
	virtual void _log_message(const String &p_message, bool p_error) override;
};

#endif // SENTRY_LOGGER_H
