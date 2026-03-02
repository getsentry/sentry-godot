#pragma once

#include <godot_cpp/classes/object.hpp>

using namespace godot;

namespace sentry {

class SentryMetrics : public Object {
	GDCLASS(SentryMetrics, Object);

protected:
	static void _bind_methods();

public:
	void count(const String &p_name, int64_t p_value, const Dictionary &p_attributes = Dictionary());
	void gauge(const String &p_name, double p_value, const String &p_unit = String(), const Dictionary &p_attributes = Dictionary());
	void distribution(const String &p_name, double p_value, const String &p_unit = String(), const Dictionary &p_attributes = Dictionary());

	SentryMetrics() = default;
};

} // namespace sentry
