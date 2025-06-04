#ifndef SENTRY_LOGGER_H
#define SENTRY_LOGGER_H

#include "sentry/godot_error_types.h"

#include <chrono>
#include <deque>
#include <godot_cpp/classes/logger.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/script_backtrace.hpp>
#include <regex>
#include <unordered_map>

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

	Ref<Mutex> mutex;

	// Stores the last time an error was logged for each source line that generated an error.
	std::unordered_map<SourceLine, TimePoint, SourceLineHash> source_line_times;

	// Time points for events captured within throttling window.
	std::deque<TimePoint> event_times;

	struct Limits {
		int events_per_frame;
		std::chrono::milliseconds repeated_error_window;
		std::chrono::milliseconds throttle_window;
		int throttle_events;
	} limits;

	// Number of events captured during this frame.
	// Note: Initialize to negative value to allow more events in first frame as it's likely to have more issues.
	int frame_events = -10;

	// Patterns that are checked against each message.
	// If matching, the message is not added as breadcrumb.
	std::vector<std::regex> filter_patterns;
	std::vector<String> filter_exact_matches;

	// Used for traceback print filtering.
	String filter_native_trace_starter_begins;
	String filter_native_trace_finisher_exact;
	std::regex filter_script_trace_starter_pattern;
	String filter_script_trace_finisher_exact;
	bool skip_logging_message = false; // Set by filtering until further condition unsets this.

	void _process_frame();

protected:
	static void _bind_methods() {}

public:
	virtual void _log_error(const String &p_function, const String &p_file, int32_t p_line, const String &p_code, const String &p_rationale, bool p_editor_notify, int32_t p_error_type, const TypedArray<ScriptBacktrace> &p_script_backtraces) override;
	virtual void _log_message(const String &p_message, bool p_error) override;

	SentryLogger();
	~SentryLogger();
};

#endif // SENTRY_LOGGER_H
