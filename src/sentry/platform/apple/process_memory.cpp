#include "sentry/platform/platform.h"

#include <mach/mach.h>

namespace sentry::platform {

bool get_process_resident_memory(uint64_t &r_bytes) {
	mach_task_basic_info_data_t info{};
	mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS) {
		return false;
	}

	r_bytes = info.resident_size;
	return true;
}

} // namespace sentry::platform
