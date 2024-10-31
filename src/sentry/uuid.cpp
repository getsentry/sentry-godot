#include "uuid.h"

#include "sentry/native/native_util.h"

namespace sentry::uuid {

String make_uuid() {
#if defined(LINUX_ENABLED) || defined(WINDOWS_ENABLED) || defined(MACOS_ENABLED)
	return sentry::native::make_uuid();
#else
	// TODO: uuid for other platforms.
	return "1234";
#endif
}

} // namespace sentry::uuid
