#pragma once

#include "sentry/log_level.h"

#include <godot_cpp/classes/object.hpp>

using namespace godot;

namespace sentry {

// Public interface for Sentry structured logging.
class SentryLogger : public Object {
	GDCLASS(SentryLogger, Object);

protected:
	static void _bind_methods();

public:
	void log(LogLevel p_level, const String &p_body);
	void trace(const String &p_body);
	void debug(const String &p_body);
	void info(const String &p_body);
	void warn(const String &p_body);
	void error(const String &p_body);
	void fatal(const String &p_body);

	SentryLogger();
};

} // namespace sentry
