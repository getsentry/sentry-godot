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
	void log(LogLevel p_level, const String &p_body, const Array &p_params, const Dictionary &p_attributes);
	void trace(const String &p_body, const Array &p_params, const Dictionary &p_attributes);
	void debug(const String &p_body, const Array &p_params, const Dictionary &p_attributes);
	void info(const String &p_body, const Array &p_params, const Dictionary &p_attributes);
	void warn(const String &p_body, const Array &p_params, const Dictionary &p_attributes);
	void error(const String &p_body, const Array &p_params, const Dictionary &p_attributes);
	void fatal(const String &p_body, const Array &p_params, const Dictionary &p_attributes);

	SentryLogger();
};

} // namespace sentry
