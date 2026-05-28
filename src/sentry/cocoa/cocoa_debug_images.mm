#include "cocoa_debug_images.h"

#include "cocoa_includes.h"

namespace sentry::cocoa {

Vector<DebugImage> get_debug_images(const int64_t *p_addresses, int32_t p_addresses_count) {
	Vector<DebugImage> images;
	if (p_addresses == nullptr || p_addresses_count <= 0) {
		return images;
	}

	SentryBinaryImageCache *cache = SentryDependencies.binaryImageCache;
	if (cache == nil) {
		return images;
	}

	for (int32_t i = 0; i < p_addresses_count; ++i) {
		SentryBinaryImageInfo *info = [cache imageByAddress:(uint64_t)p_addresses[i]];
		if (info == nil) {
			continue;
		}

		DebugImage image;
		image.code_file = String::utf8([info.name UTF8String]);
		image.debug_id = info.uuid ? String::utf8([info.uuid UTF8String]) : String();
		image.image_address = static_cast<int64_t>(info.address);
		image.image_size = static_cast<int64_t>(info.size);
		images.push_back(image);
	}
	return images;
}

} // namespace sentry::cocoa
