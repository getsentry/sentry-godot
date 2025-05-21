#include "uuid.h"

#ifdef NATIVE_SDK
#include "sentry/native/native_util.h"
#endif

#ifdef COCOA_SDK
#include "sentry/cocoa/cocoa_uuid.h"
#endif

namespace sentry::uuid {

String make_uuid() {
#if defined(NATIVE_SDK)
	return sentry::native::make_uuid();
#elif defined(COCOA_SDK)
    return sentry::cocoa::make_uuid();
#else
	// TODO: uuid for other platforms.
	ERR_FAIL_V_MSG("", "Sentry: UUID not supported on this platform.");
#endif
}

} // namespace sentry::uuid
