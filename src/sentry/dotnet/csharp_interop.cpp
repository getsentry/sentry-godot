#include "gen/sdk_version.gen.h"
#include "sentry/environment.h"
#include "sentry/sentry_sdk.h"

#ifdef _WIN32
#define CSHARP_EXPORT __declspec(dllexport)
#else
#define CSHARP_EXPORT __attribute__((visibility("default")))
#endif

using namespace sentry;

extern "C" {

// Native-owned string handle for passing Godot Strings across the interop boundary.
// C# must call csharp_interop_string_free() after reading the data.
struct GodotStringHandle {
	const char32_t *ptr;
	int64_t len;
	void *handle; // Opaque: heap-allocated String* that owns the refcount.
};

GodotStringHandle _make_handle(const String &p_str) {
	String *owned = memnew(String(p_str));
	return { owned->ptr(), owned->length(), (void *)owned };
}

CSHARP_EXPORT void csharp_interop_string_free(void *p_handle) {
	if (p_handle) {
		memdelete((String *)p_handle);
	}
}

// Managed-owned string map for passing Dictionary<string, string> across P/Invoke.
// C# builds and pins this; native reads it synchronously during the call.
// Buffer layout: key1+val1+key2+val2... concatenated as UTF-16.
// Lengths array: key1_len, val1_len, key2_len, val2_len, ... (2*pair_count entries).
struct ManagedStringMap {
	const char16_t *buffer;
	const int32_t *lengths;
	int32_t pair_count;
};

// Decode a ManagedStringMap into a Godot Dictionary (String -> String).
static Dictionary _managed_string_map_to_dictionary(const ManagedStringMap &map) {
	Dictionary dict;
	const char16_t *ptr = map.buffer;
	for (int32_t i = 0; i < map.pair_count; i++) {
		int32_t key_len = map.lengths[i * 2];
		int32_t val_len = map.lengths[i * 2 + 1];
		String key = String::utf16(ptr, key_len);
		ptr += key_len;
		String val = String::utf16(ptr, val_len);
		ptr += val_len;
		dict[key] = val;
	}
	return dict;
}

CSHARP_EXPORT GodotStringHandle csharp_interop_detect_environment() {
	return _make_handle(environment::detect_godot_environment());
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
