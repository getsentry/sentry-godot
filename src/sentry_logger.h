#ifndef SENTRY_LOGGER_H
#define SENTRY_LOGGER_H

#include <fstream>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class SentryLogger : public Node {
	GDCLASS(SentryLogger, Node)
public:
	// Godot Engine logger error types.
	enum ErrorType {
		ERROR_TYPE_ERROR,
		ERROR_TYPE_WARNING,
		ERROR_TYPE_SCRIPT,
		ERROR_TYPE_SHADER,
	};

private:
	Callable process_log;
	std::ifstream log_file;

	void _setup();
	void _process_log_file();
	void _log_error(const char *p_func, const char *p_file, int p_line, const char *p_rationale, ErrorType error_type);

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
	SentryLogger();
};

#endif // SENTRY_LOGGER_H
