#include "sentry/sentry_sdk.h"

#ifdef _WIN32
#define CSHARP_EXPORT __declspec(dllexport)
#else
#define CSHARP_EXPORT __attribute__((visibility("default")))
#endif

using namespace sentry;

extern "C" {

CSHARP_EXPORT void csharp_interop_sdk_set_tag(const char16_t *key, int key_len,
		const char16_t *value, int value_len) {
	String k = String::utf16(key, key_len);
	String v = String::utf16(value, value_len);
	SentrySDK::get_singleton()->set_tag(k, v);
	print_line("SUCCESS - YAY!");
}

} // extern "C"
