#ifndef SENTRY_LOGGER_H
#define SENTRY_LOGGER_H

#include "sentry/godot_error_types.h"

#include <chrono>
#include <deque>
#include <godot_cpp/classes/logger.hpp>
#include <godot_cpp/classes/script_backtrace.hpp>
#include <mutex>
#include <unordered_map>

using namespace godot;

namespace sentry {

class SentryGodotLogger : public Logger {
	GDCLASS(SentryGodotLogger, Logger);

private:
	using GodotErrorType = sentry::GodotErrorType;
	using TimePoint = std::chrono::high_resolution_clock::time_point;

	String logger_name;
	Dictionary log_attributes;

	struct Limits {
		int events_per_frame;
		std::chrono::milliseconds repeated_error_window;
		std::chrono::milliseconds throttle_window;
		int throttle_events;
	} limits;

	struct ErrorKey {
		String message;
		String file;
		int line;

		bool operator==(const ErrorKey &p_other) const {
			return message == p_other.message && file == p_other.file && line == p_other.line;
		}
	};

	struct ErrorKeyHash {
		std::size_t operator()(const ErrorKey &p_key) const;
	};

	std::mutex error_mutex;

	// Stores the last time an error was logged for each source line that generated an error.
	std::unordered_map<ErrorKey, TimePoint, ErrorKeyHash> error_timepoints;

	// Time points for events captured within throttling window.
	std::deque<TimePoint> event_times;

	// Number of events captured during this frame.
	int frame_events = 0;

	// Filter: Remove messages from breadcrumbs which start with any of these prefixes.
	std::vector<String> filter_by_prefix;

	void _connect_process_frame();
	void _disconnect_process_frame();
	void _process_frame();

protected:
	static void _bind_methods();

	void _notification(int p_what);

public:
	virtual void _log_error(const String &p_function, const String &p_file, int32_t p_line, const String &p_code, const String &p_rationale, bool p_editor_notify, int32_t p_error_type, const TypedArray<Ref<ScriptBacktrace>> &p_script_backtraces) override;
	virtual void _log_message(const String &p_message, bool p_error) override;

	SentryGodotLogger();
	~SentryGodotLogger();
};

} // namespace sentry

#endif // SENTRY_LOGGER_H
