#ifndef SENTRY_LEVEL_H
#define SENTRY_LEVEL_H

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/object.hpp>
using namespace godot;

namespace sentry {

enum Level {
	LEVEL_DEBUG = 0,
	LEVEL_INFO = 1,
	LEVEL_WARNING = 2,
	LEVEL_ERROR = 3,
	LEVEL_FATAL = 4
};

CharString level_as_cstring(Level level);

} // namespace sentry

#endif // SENTRY_LEVEL_H
