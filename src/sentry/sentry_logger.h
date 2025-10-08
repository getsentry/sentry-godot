#pragma once

#include "sentry/level.h"

#include <godot_cpp/classes/object.hpp>

using namespace godot;

namespace sentry {

// Public interface for Sentry structured logging.
class SentryLogger : public Object {
	GDCLASS(SentryLogger, Object);

protected:
	static void _bind_methods();

public:
	void log(sentry::Level p_level, const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void debug(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void info(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void warn(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void error(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void fatal(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());

	SentryLogger();
};

} // namespace sentry
