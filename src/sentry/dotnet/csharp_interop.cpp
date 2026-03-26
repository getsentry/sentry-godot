#include "gen/sdk_version.gen.h"
#include "sentry/environment.h"
#include "sentry/sentry_sdk.h"

#ifdef _WIN32
#define CSHARP_EXPORT __declspec(dllexport)
#else
#define CSHARP_EXPORT __attribute__((visibility("default")))
#endif

using namespace sentry;

constexpr int INTEROP_SMALL_STRING_LEN = 64;

extern "C" {

struct InteropString {
	const char32_t *ptr;
	int64_t len;
};

CSHARP_EXPORT InteropString csharp_interop_detect_environment() {
	const String &env = environment::detect_godot_environment();
	return { env.ptr(), env.length() };
}

CSHARP_EXPORT void csharp_interop_sdk_set_tag(const char16_t *key, int key_len,
		const char16_t *value, int value_len) {
	String k = String::utf16(key, key_len);
	String v = String::utf16(value, value_len);
	SentrySDK::get_singleton()->set_tag(k, v);
}

CSHARP_EXPORT void csharp_interop_sdk_remove_tag(const char16_t *key, int key_len) {
	String k = String::utf16(key, key_len);
	SentrySDK::get_singleton()->remove_tag(k);
}

CSHARP_EXPORT void csharp_interop_set_trace(const char *p_trace_id, const char *p_parent_span_id) {
	SentrySDK::get_singleton()->set_trace(p_trace_id, p_parent_span_id);
}

CSHARP_EXPORT const char *csharp_interop_get_sdk_version() {
	return SENTRY_GODOT_SDK_VERSION;
}

} // extern "C"
