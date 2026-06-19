#pragma once

#include <godot_cpp/classes/object.hpp>

using namespace godot;

namespace sentry {

// Deliberately crashes the application to test crash reporting.
class SentryBadCode : public Object {
	GDCLASS(SentryBadCode, Object);

protected:
	static void _bind_methods();

public:
	void crash_with_null_dereference();
	void crash_with_stack_overflow();
	void crash_with_abort();
	void crash_with_division_by_zero();

	SentryBadCode() = default;
};

} // namespace sentry
