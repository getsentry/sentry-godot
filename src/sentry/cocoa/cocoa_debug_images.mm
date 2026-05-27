#include "cocoa_debug_images.h"

#include "cocoa_includes.h"

namespace sentry::cocoa {

int32_t get_debug_images(const int64_t *p_addresses, int32_t p_addresses_count, VisitImageFunc p_callback, void *p_userdata) {
	if (p_callback == nullptr || p_addresses == nullptr || p_addresses_count <= 0) {
		return 0;
	}

	SentryBinaryImageCache *cache = SentryDependencies.binaryImageCache;
	if (cache == nil) {
		return 0;
	}

	int32_t emitted = 0;
	for (int32_t i = 0; i < p_addresses_count; ++i) {
		SentryBinaryImageInfo *info = [cache imageByAddress:(uint64_t)p_addresses[i]];
		if (info == nil) {
			continue;
		}

		MachOImage image = {
			info.name ? [info.name UTF8String] : "",
			info.uuid ? [info.uuid UTF8String] : "",
			static_cast<int64_t>(info.address),
			static_cast<int64_t>(info.size),
		};
		p_callback(&image, p_userdata);
		++emitted;
	}
	return emitted;
}

} // namespace sentry::cocoa
