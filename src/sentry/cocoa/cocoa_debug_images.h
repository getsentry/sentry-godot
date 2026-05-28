#pragma once

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>

using namespace godot;

namespace sentry::cocoa {

// Mach-O debug image resolved from sentry-cocoa's in-process binary image cache.
struct DebugImage {
	String code_file;
	String debug_id;
	int64_t image_address = 0; // load address in the process
	int64_t image_size = 0; // 0 if unknown
};

// Finds Mach-O images by base address in sentry-cocoa's in-process cache.
// Returns one entry per matched address; unmatched addresses are skipped.
Vector<DebugImage> get_debug_images(const int64_t *p_addresses, int32_t p_addresses_count);

} // namespace sentry::cocoa
