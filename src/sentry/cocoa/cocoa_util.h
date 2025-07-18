#ifndef COCOA_UTIL_H
#define COCOA_UTIL_H

#include "cocoa_includes.h"
#include "sentry/level.h"

namespace sentry::cocoa {

_FORCE_INLINE_ objc::SentryLevel sentry_level_to_objc(sentry::Level p_level) {
	switch (p_level) {
		case sentry::Level::LEVEL_DEBUG:
			return kSentryLevelDebug;
		case sentry::Level::LEVEL_INFO:
			return kSentryLevelInfo;
		case sentry::Level::LEVEL_WARNING:
			return kSentryLevelWarning;
		case sentry::Level::LEVEL_ERROR:
			return kSentryLevelError;
		case sentry::Level::LEVEL_FATAL:
			return kSentryLevelFatal;
		default:
			return kSentryLevelError;
	}
}

} //namespace sentry::cocoa

#endif // COCOA_UTIL_H
