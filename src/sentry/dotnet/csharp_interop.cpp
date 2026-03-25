#include "sentry/environment.h"
#include "sentry/sentry_sdk.h"
#include <cstring>

#ifdef _WIN32
#define CSHARP_EXPORT __declspec(dllexport)
#else
#define CSHARP_EXPORT __attribute__((visibility("default")))
#endif

using namespace sentry;

constexpr int INTEROP_SMALL_STRING_LEN = 64;

extern "C" {

CSHARP_EXPORT void csharp_interop_sdk_set_tag(const char16_t *key, int key_len,
		const char16_t *value, int value_len) {
	String k = String::utf16(key, key_len);
	String v = String::utf16(value, value_len);
	SentrySDK::get_singleton()->set_tag(k, v);
}

CSHARP_EXPORT int csharp_interop_detect_environment(char32_t *r_buffer, int p_len) {
	ERR_FAIL_COND_V(p_len < INTEROP_SMALL_STRING_LEN, -1);
	String env = environment::detect_godot_environment();
	int64_t env_len = env.length();
	ERR_FAIL_COND_V(env_len >= INTEROP_SMALL_STRING_LEN, -1);
	memcpy(r_buffer, env.ptr(), env_len * sizeof(char32_t));
	return env_len;
}

} // extern "C"
