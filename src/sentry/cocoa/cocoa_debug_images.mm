#include "cocoa_debug_images.h"

#include "sentry/cocoa/cocoa_includes.h"
#include "sentry/cocoa/cocoa_util.h"

namespace sentry::cocoa {

Vector<DebugImage> get_debug_images(const int64_t *p_addresses, int32_t p_addresses_count) {
	Vector<DebugImage> images;
	if (p_addresses == nullptr || p_addresses_count <= 0) {
		return images;
	}

	NSMutableArray<NSNumber *> *addresses = [NSMutableArray arrayWithCapacity:p_addresses_count];
	for (int32_t i = 0; i < p_addresses_count; ++i) {
		[addresses addObject:uint64_to_objc(p_addresses[i])];
	}

	NSArray<SentryObjCDebugMeta *> *objc_images = [SentryObjCSDK.internal.debug imagesForAddresses:addresses];
	for (SentryObjCDebugMeta *info in objc_images) {
		DebugImage image;
		image.code_file = string_from_objc(info.codeFile);
		image.debug_id = string_from_objc(info.debugID);
		image.image_address = static_cast<int64_t>(info.imageAddressRaw);
		image.image_size = info.imageSize.longLongValue;
		images.push_back(image);
	}
	return images;
}

} // namespace sentry::cocoa
