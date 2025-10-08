#pragma once

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

// Represents Sentry log entry.
class SentryLog : public RefCounted {
	GDCLASS(SentryLog, RefCounted);

protected:
	static void _bind_methods() {}
};

} //namespace sentry
