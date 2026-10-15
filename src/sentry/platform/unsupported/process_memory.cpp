#include "sentry/platform/platform.h"

namespace sentry::platform {

bool get_process_resident_memory(uint64_t &) {
	return false;
}

} // namespace sentry::platform
