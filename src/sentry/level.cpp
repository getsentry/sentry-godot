#include "level.h"

namespace sentry {

CharString level_as_cstring(Level level) {
	switch (level) {
		case Level::LEVEL_DEBUG:
			return "debug";
		case Level::LEVEL_INFO:
			return "info";
		case Level::LEVEL_WARNING:
			return "warning";
		case Level::LEVEL_ERROR:
			return "error";
		case Level::LEVEL_FATAL:
			return "fatal";
		default:
			return "unknown";
	}
}

} // namespace sentry
