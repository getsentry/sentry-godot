#include "sentry/platform/platform.h"

#include <unistd.h>
#include <cinttypes>
#include <cstdio>

namespace sentry::platform {

bool get_process_resident_memory(uint64_t &r_bytes) {
	const long page_size = sysconf(_SC_PAGESIZE);
	if (page_size <= 0) {
		return false;
	}

	FILE *file = fopen("/proc/self/statm", "r");
	if (file == nullptr) {
		return false;
	}

	uint64_t resident_pages = 0;
	const int fields = fscanf(file, "%*s %" SCNu64, &resident_pages);
	fclose(file);
	if (fields != 1) {
		return false;
	}

	r_bytes = resident_pages * static_cast<uint64_t>(page_size);
	return true;
}

} // namespace sentry::platform
