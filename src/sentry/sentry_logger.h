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
	void log(LogLevel p_level, const String &p_body, const Dictionary &p_params);
	void trace(const String &p_body, const Dictionary &p_params);
	void debug(const String &p_body, const Dictionary &p_params);
	void info(const String &p_body, const Dictionary &p_params);
	void warn(const String &p_body, const Dictionary &p_params);
	void error(const String &p_body, const Dictionary &p_params);
	void fatal(const String &p_body, const Dictionary &p_params);

	SentryLogger();
};

} // namespace sentry
