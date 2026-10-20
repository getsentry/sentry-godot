#pragma once

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace sentry::android {

godot::Variant sanitize_variant(const godot::Variant &p_value, int p_depth = 0);

namespace detail {

void print_missing_plugin_error(const char *p_function, const char *p_file, int p_line);

} // namespace detail

} // namespace sentry::android

// These declare a local from _android_plugin; use inside a braced block.
#define ENSURE_ANDROID_PLUGIN(m_plugin)                                                          \
	::godot::Object *m_plugin = _android_plugin.get();                                           \
	if (unlikely(m_plugin == nullptr)) {                                                         \
		::sentry::android::detail::print_missing_plugin_error(FUNCTION_STR, __FILE__, __LINE__); \
		return;                                                                                  \
	} else                                                                                       \
		((void)0)

#define ENSURE_ANDROID_PLUGIN_V(m_plugin, m_retval)                                              \
	::godot::Object *m_plugin = _android_plugin.get();                                           \
	if (unlikely(m_plugin == nullptr)) {                                                         \
		::sentry::android::detail::print_missing_plugin_error(FUNCTION_STR, __FILE__, __LINE__); \
		return m_retval;                                                                         \
	} else                                                                                       \
		((void)0)
