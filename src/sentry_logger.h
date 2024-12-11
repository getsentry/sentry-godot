#ifndef SENTRY_LOGGER_H
#define SENTRY_LOGGER_H

#include "sentry/godot_error_types.h"
#include "sentry/level.h"

#include <chrono>
#include <deque>
#include <fstream>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <unordered_map>

using namespace godot;

class SentryLogger : public Node {
	GDCLASS(SentryLogger, Node)

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

	// Number of events captured during this frame.
	int frame_events = 0;

	Callable process_log;
	std::ifstream log_file;
	Timer *trim_timer = nullptr;

	void _setup();
	void _process_log_file();
	void _log_error(const char *p_func, const char *p_file, int p_line, const char *p_rationale, GodotErrorType error_type);
	void _trim_error_timepoints();

	// Returns true if an error occurred. Populates the last three arguments passed by reference.
	bool _get_script_context(const String &p_file, int p_line, String &r_context_line, PackedStringArray &r_pre_context, PackedStringArray &r_post_context) const;

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	SentryLogger();
};

#endif // SENTRY_LOGGER_H
