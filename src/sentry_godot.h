#ifndef SENTRY_GODOT_H
#define SENTRY_GODOT_H

#include <gdextension_interface.h>

#if !defined(GDE_EXPORT)
#if defined(_WIN32)
#define GDE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
#define GDE_EXPORT __attribute__((visibility("default")))
#else
#define GDE_EXPORT
#endif
#endif
#ifdef __cplusplus
extern "C" {
#endif

GDExtensionBool GDE_EXPORT sentry_godot_gdextension_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization);

#ifdef __cplusplus
}
#endif

#endif // SENTRY_GODOT_H