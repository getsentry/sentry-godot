#include "uuid.h"

#ifdef NATIVE_SDK
#include "sentry/native/native_util.h"
#endif

namespace sentry::uuid {

String make_uuid() {
#ifdef NATIVE_SDK
	return sentry::native::make_uuid();
#else
	// TODO: uuid for other platforms.
	return "1234";
#endif
}

} // namespace sentry::uuid
