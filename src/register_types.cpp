#include "editor/sentry_editor_plugin.h"
#include "sentry/disabled/disabled_event.h"
#include "sentry/logging/sentry_godot_logger.h"
#include "sentry/processing/screenshot_processor.h"
#include "sentry/processing/sentry_event_processor.h"
#include "sentry/processing/view_hierarchy_processor.h"
#include "sentry/runtime_config.h"
#include "sentry/sentry_attachment.h"
#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_event.h"
#include "sentry/sentry_feedback.h"
#include "sentry/sentry_log.h"
#include "sentry/sentry_logger.h"
#include "sentry/sentry_options.h"
#include "sentry/sentry_sdk.h"
#include "sentry/sentry_user.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

#ifdef SDK_NATIVE
#include "sentry/native/native_breadcrumb.h"
#include "sentry/native/native_event.h"
#include "sentry/native/native_log.h"
#endif // SDK_NATIVE

#ifdef SDK_ANDROID
#include "sentry/android/android_breadcrumb.h"
#include "sentry/android/android_event.h"
#include "sentry/android/android_log.h"
#include "sentry/android/android_sdk.h"
#endif // SDK_ANDROID

#ifdef SDK_COCOA
#include "sentry/cocoa/cocoa_breadcrumb.h"
#include "sentry/cocoa/cocoa_event.h"
#include "sentry/cocoa/cocoa_log.h"
#endif // SDK_COCOA

#ifdef SDK_JAVASCRIPT
#include "sentry/javascript/javascript_breadcrumb.h"
#include "sentry/javascript/javascript_event.h"
#include "sentry/javascript/javascript_log.h"
#include "sentry/javascript/javascript_sdk.h"
#endif

#ifdef TOOLS_ENABLED
#include "editor/sentry_editor_export_plugin_android.h"
#include "editor/sentry_editor_export_plugin_unix.h"
#include "editor/sentry_editor_export_plugin_web.h"
#include "editor/sentry_editor_plugin.h"
#include <godot_cpp/classes/editor_plugin_registration.hpp>
#endif // TOOLS_ENABLED

using namespace godot;
using namespace sentry;

void register_runtime_classes() {
	GDREGISTER_CLASS(SentryLoggerLimits);
	GDREGISTER_CLASS(SentryExperimental);
	GDREGISTER_CLASS(SentryOptions);
	GDREGISTER_INTERNAL_CLASS(RuntimeConfig);
	GDREGISTER_CLASS(SentryUser);
	GDREGISTER_CLASS(SentryTimestamp);
	GDREGISTER_CLASS(SentryLogger);
	GDREGISTER_CLASS(SentryFeedback);
	GDREGISTER_CLASS(SentrySDK);
	GDREGISTER_ABSTRACT_CLASS(SentryAttachment);
	GDREGISTER_ABSTRACT_CLASS(SentryEvent);
	GDREGISTER_ABSTRACT_CLASS(SentryBreadcrumb);
	GDREGISTER_ABSTRACT_CLASS(SentryLog);
	GDREGISTER_INTERNAL_CLASS(DisabledEvent);
	GDREGISTER_INTERNAL_CLASS(SentryEventProcessor);
	GDREGISTER_INTERNAL_CLASS(ScreenshotProcessor);
	GDREGISTER_INTERNAL_CLASS(ViewHierarchyProcessor);
	GDREGISTER_INTERNAL_CLASS(logging::SentryGodotLogger);

#ifdef SDK_NATIVE
	GDREGISTER_INTERNAL_CLASS(native::NativeEvent);
	GDREGISTER_INTERNAL_CLASS(native::NativeBreadcrumb);
	GDREGISTER_INTERNAL_CLASS(native::NativeLog);
#endif

#ifdef SDK_ANDROID
	GDREGISTER_INTERNAL_CLASS(android::AndroidEvent);
	GDREGISTER_INTERNAL_CLASS(android::AndroidBreadcrumb);
	GDREGISTER_INTERNAL_CLASS(android::AndroidLog);
	GDREGISTER_INTERNAL_CLASS(android::SentryAndroidBeforeSendHandler);
	GDREGISTER_INTERNAL_CLASS(android::SentryAndroidBeforeSendLogHandler);
#endif

#ifdef SDK_COCOA
	GDREGISTER_INTERNAL_CLASS(cocoa::CocoaEvent);
	GDREGISTER_INTERNAL_CLASS(cocoa::CocoaBreadcrumb);
	GDREGISTER_INTERNAL_CLASS(cocoa::CocoaLog);
#endif

#ifdef SDK_JAVASCRIPT
	GDREGISTER_INTERNAL_CLASS(javascript::JavaScriptEvent);
	GDREGISTER_INTERNAL_CLASS(javascript::JavaScriptBreadcrumb);
	GDREGISTER_INTERNAL_CLASS(javascript::JavaScriptBeforeSendHandler);
	GDREGISTER_INTERNAL_CLASS(javascript::JavaScriptLog);
	GDREGISTER_INTERNAL_CLASS(javascript::JavaScriptBeforeSendLogHandler);
#endif
}

void register_editor_classes() {
#ifdef TOOLS_ENABLED
	GDREGISTER_INTERNAL_CLASS(SentryEditorExportPluginAndroid);
	GDREGISTER_INTERNAL_CLASS(SentryEditorExportPluginWeb);
	GDREGISTER_INTERNAL_CLASS(SentryEditorPlugin);

#ifndef WINDOWS_ENABLED
	GDREGISTER_INTERNAL_CLASS(SentryEditorExportPluginUnix);
#endif // !WINDOWS_ENABLED

#endif // TOOLS_ENABLED
}

void initialize_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		register_runtime_classes();
		SentrySDK::create_singleton();
		SentrySDK::get_singleton()->prepare_and_auto_initialize();
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
#ifdef TOOLS_ENABLED
		register_editor_classes();
		EditorPlugins::add_by_type<SentryEditorPlugin>();
#endif // TOOLS_ENABLED
	}
}

void uninitialize_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		SentrySDK::destroy_singleton();
	}
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT sentry_gdextension_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_module);
	init_obj.register_terminator(uninitialize_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
