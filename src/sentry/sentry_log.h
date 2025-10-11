#pragma once

#include "sentry/log_level.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/binder_common.hpp>

using namespace godot;

namespace sentry {

// Represents Sentry structured log entry.
class SentryLog : public RefCounted {
	GDCLASS(SentryLog, RefCounted);

public:
	using LogLevel = ::sentry::LogLevel;

protected:
	static void _bind_methods();

public:
	virtual LogLevel get_level() const = 0;
	virtual void set_level(LogLevel p_level) = 0;

	virtual String get_body() const = 0;
	virtual void set_body(const String &p_body) = 0;

	virtual Variant get_attribute(const String &p_name) const = 0;
	virtual void set_attribute(const String &p_name, const Variant &p_value) = 0;
	virtual void add_attributes(const Dictionary &p_attributes) = 0;
	virtual void remove_attribute(const String &p_name) = 0;

	virtual ~SentryLog() = default;
};

} //namespace sentry

VARIANT_ENUM_CAST(sentry::SentryLog::LogLevel);
