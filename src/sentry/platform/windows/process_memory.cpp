#include "sentry/platform/platform.h"

#include <windows.h>

#include <psapi.h>

namespace sentry::platform {

bool get_process_resident_memory(uint64_t &r_bytes) {
	PROCESS_MEMORY_COUNTERS counters{};
	if (!K32GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
		return false;
	}

	r_bytes = counters.WorkingSetSize;
	return true;
}

} // namespace sentry::platform
