#pragma once

#include <cstdint>

namespace sentry::cocoa {

// Mach-O debug image data.
// Strings are owned by cocoa, UTF-8, and valid only inside the callback.
struct MachOImage {
	const char *code_file; // never null
	const char *debug_id; // never null
	int64_t image_address; // load address in the process
	int64_t image_size; // 0 if unknown
};

typedef void (*VisitImageFunc)(const MachOImage *p_image, void *p_userdata);

// Finds Mach-O images by base address in sentry-cocoa's in-process cache and
// invokes the callback once per match. Returns the number of emitted images.
// User data is passed through to the callback.
int32_t get_debug_images(const int64_t *p_addresses, int32_t p_addresses_count, VisitImageFunc p_callback, void *p_userdata);

} // namespace sentry::cocoa
