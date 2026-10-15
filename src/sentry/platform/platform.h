#pragma once

#include <cstdint>

namespace sentry::platform {

// Queries process RSS (working set on Windows) in bytes.
// Leaves r_bytes unchanged on failure or unsupported platforms.
bool get_process_resident_memory(uint64_t &r_bytes);

} // namespace sentry::platform
