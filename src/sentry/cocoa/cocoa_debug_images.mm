#include "cocoa_debug_images.h"

#include "cocoa_includes.h"

// WORKAROUND: The SentryObjC SDK has no public or internal API to look up binary images by address,
// which we need to symbolicate .NET exceptions.
// The types that do this are still inside the framework binary, just not declared in any header.
// We redeclare the few bits we call here so the code compiles and links against them.
// Remove once the SDK exposes an official API.
// Upstream issue: https://github.com/getsentry/sentry-cocoa/issues/8039

@interface SentryBinaryImageInfo : NSObject
@property(nonatomic, readonly) NSString *name;
@property(nonatomic, readonly) NSString *uuid;
@property(nonatomic, readonly) uint64_t address;
@property(nonatomic, readonly) uint64_t size;
@end

@interface SentryBinaryImageCache : NSObject
- (SentryBinaryImageInfo *)imageByAddress:(uint64_t)address;
@end

@interface SentryDependencies : NSObject
@property(class, nonatomic, readonly) SentryBinaryImageCache *binaryImageCache;
@end

// END OF WORKAROUND

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
		image.code_file = String::utf8([info.name UTF8String]); // info.name is nonnull
		image.debug_id = info.uuid ? String::utf8([info.uuid UTF8String]) : String();
		image.image_address = static_cast<int64_t>(info.address);
		image.image_size = static_cast<int64_t>(info.size);
		images.push_back(image);
	}
	return images;
}

} // namespace sentry::cocoa
