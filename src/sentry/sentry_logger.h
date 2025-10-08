#pragma once

#include "sentry/log_level.h"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/binder_common.hpp>

using namespace godot;

namespace sentry {

// Public interface for Sentry structured logging.
class SentryLogger : public Object {
	GDCLASS(SentryLogger, Object);

public:
	using LogLevel = ::sentry::LogLevel;

protected:
	static void _bind_methods();

public:
	void log(LogLevel p_level, const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void trace(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void debug(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void info(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void warn(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void error(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());
	void fatal(const String &p_body, const Array &p_params = Array(), const Dictionary &p_attributes = Dictionary());

	SentryLogger();
};

} // namespace sentry

VARIANT_ENUM_CAST(sentry::SentryLogger::LogLevel);
