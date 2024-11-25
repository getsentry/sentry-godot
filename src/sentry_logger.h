#ifndef SENTRY_LOGGER_H
#define SENTRY_LOGGER_H

#include "sentry/godot_error_types.h"
#include "sentry/level.h"

#include <fstream>
#include <godot_cpp/classes/node.hpp>

using namespace godot;

class SentryLogger : public Node {
	GDCLASS(SentryLogger, Node)
private:
	using GodotErrorType = sentry::GodotErrorType;

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
