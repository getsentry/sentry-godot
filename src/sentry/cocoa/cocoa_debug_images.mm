#include "cocoa_debug_images.h"

#include "cocoa_includes.h"

namespace sentry::cocoa {

Vector<DebugImage> get_debug_images(const int64_t *p_addresses, int32_t p_addresses_count) {
	// GAP: SentryObjC wrapper SDK doesn't expose SentryDependencies.binaryImageCache / SentryBinaryImageInfo,
	//      which were used to resolve debug images for .NET exception symbolication.
	//      In the main Cocoa package, these are internal SPI classes (the upstream .NET SDK consumes these in a
	//      similar way as we do in Godot).
	// TODO: Needs upstream exposure.
	WARN_PRINT_ONCE("Sentry: Debug image resolution is currently not supported with the SentryObjC SDK.");
	return Vector<DebugImage>();

	// Vector<DebugImage> images;
	// if (p_addresses == nullptr || p_addresses_count <= 0) {
	// 	return images;
	// }

	// SentryBinaryImageCache *cache = SentryDependencies.binaryImageCache;
	// if (cache == nil) {
	// 	return images;
	// }

	// for (int32_t i = 0; i < p_addresses_count; ++i) {
	// 	SentryBinaryImageInfo *info = [cache imageByAddress:(uint64_t)p_addresses[i]];
	// 	if (info == nil) {
	// 		continue;
	// 	}

	// 	DebugImage image;
	// 	image.code_file = String::utf8([info.name UTF8String]); // info.name is nonnull
	// 	image.debug_id = info.uuid ? String::utf8([info.uuid UTF8String]) : String();
	// 	image.image_address = static_cast<int64_t>(info.address);
	// 	image.image_size = static_cast<int64_t>(info.size);
	// 	images.push_back(image);
	// }
	// return images;
}

} // namespace sentry::cocoa
