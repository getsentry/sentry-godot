#include "editor/sentry_editor_export_plugin_unix.h"
#include "editor/sentry_editor_plugin.h"
#include "runtime_config.h"
#include "sentry/disabled_event.h"
#include "sentry_configuration.h"
#include "sentry_event.h"
#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_sdk.h"
#include "sentry_user.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

#ifdef NATIVE_SDK
#include "sentry/native/native_event.h"
#endif // NATIVE_SDK

#ifdef ANDROID_ENABLED
#include "sentry/android/android_event.h"
#include "sentry/android/android_sdk.h"
#endif // ANDROID_ENABLED

#ifdef TOOLS_ENABLED
#include "editor/sentry_editor_export_plugin_android.h"
#include "editor/sentry_editor_plugin.h"
#include <godot_cpp/classes/editor_plugin_registration.hpp>
#endif // TOOLS_ENABLED

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
	} else if (p_level == godot::MODULE_INITIALIZATION_LEVEL_SERVERS) {
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(SentryLoggerLimits);
		GDREGISTER_CLASS(SentryOptions);
		GDREGISTER_INTERNAL_CLASS(RuntimeConfig);
		GDREGISTER_CLASS(SentryConfiguration);
		GDREGISTER_CLASS(SentryUser);
		GDREGISTER_CLASS(SentrySDK);
		GDREGISTER_ABSTRACT_CLASS(SentryEvent);
		GDREGISTER_INTERNAL_CLASS(DisabledEvent);
		GDREGISTER_INTERNAL_CLASS(SentryLogger);

#ifdef NATIVE_SDK
		GDREGISTER_INTERNAL_CLASS(NativeEvent);
#endif

#ifdef ANDROID_ENABLED
		GDREGISTER_INTERNAL_CLASS(AndroidEvent);

		{
			using namespace sentry;
			GDREGISTER_INTERNAL_CLASS(SentryAndroidBeforeSendHandler);
		}
#endif

		SentryOptions::create_singleton();

		SentrySDK *sentry_singleton = memnew(SentrySDK);
		Engine::get_singleton()->register_singleton("SentrySDK", SentrySDK::get_singleton());
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
#ifdef TOOLS_ENABLED
#ifndef WINDOWS_ENABLED
		GDREGISTER_INTERNAL_CLASS(SentryEditorExportPluginUnix);
#endif // !WINDOWS_ENABLED
		GDREGISTER_INTERNAL_CLASS(SentryEditorExportPluginAndroid);
		GDREGISTER_INTERNAL_CLASS(SentryEditorPlugin);
		EditorPlugins::add_by_type<SentryEditorPlugin>();
#endif // TOOLS_ENABLED
	}
}

void uninitialize_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		if (SentrySDK::get_singleton()) {
			memdelete(SentrySDK::get_singleton());
		}
		SentryOptions::destroy_singleton();
	}
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT gdextension_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_module);
	init_obj.register_terminator(uninitialize_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
