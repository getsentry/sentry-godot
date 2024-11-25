#ifndef SENTRY_LOGGER_H
#define SENTRY_LOGGER_H

#include "sentry/godot_error_types.h"
#include "sentry/level.h"

#include <chrono>
#include <fstream>
#include <godot_cpp/classes/node.hpp>
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

	// Stores the last time an error was logged for each source line.
	std::unordered_map<SourceLine, TimePoint, SourceLineHash> last_logged;

	Callable process_log;
	std::ifstream log_file;

	int num_breadcrumbs_captured = 0;
	int num_events_captured = 0;

	void _setup();
	void _process_log_file();
	void _log_error(const char *p_func, const char *p_file, int p_line, const char *p_rationale, GodotErrorType error_type);

	// Returns true if an error occurred. Populates the last three arguments passed by reference.
	bool _get_script_context(const String &p_file, int p_line, String &r_context_line, PackedStringArray &r_pre_context, PackedStringArray &r_post_context) const;

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	SentryLogger();
};

#endif // SENTRY_LOGGER_H
