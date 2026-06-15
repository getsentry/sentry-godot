#include "library_path.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else // !_WIN32
#include <dlfcn.h>
#endif

namespace sentry::util {

godot::String get_loaded_gdextension_library_path() {
	// NOTE: In the Windows editor, Godot loads a temporary "~"-prefixed copy of the library,
	// while gdextension_interface_get_library_path() reports the original path.
	// Passing that path to C# DllImport resolver would load a second, uninitialized copy of the library
	// (with a null SentrySDK singleton), so resolve the module that is actually loaded instead.
	// See issue 761.

#if defined(_WIN32)
	HMODULE module = nullptr;
	if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				reinterpret_cast<LPCWSTR>(&get_loaded_gdextension_library_path),
				&module) &&
			module != nullptr) {
		WCHAR buffer[4096];
		DWORD length = GetModuleFileNameW(module, buffer, static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])));
		if (length > 0 && length < sizeof(buffer) / sizeof(buffer[0])) {
			return godot::String::utf16(reinterpret_cast<const char16_t *>(buffer), static_cast<int>(length));
		}
	}
	return godot::String();
#else
	Dl_info info = {};
	if (dladdr(reinterpret_cast<const void *>(&get_loaded_gdextension_library_path), &info) != 0 && info.dli_fname != nullptr) {
		return godot::String::utf8(info.dli_fname);
	}
	return godot::String();
#endif
}

} // namespace sentry::util
