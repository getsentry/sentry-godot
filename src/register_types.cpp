#include "runtime_config.h"
#include "sentry/disabled_event.h"
#include "sentry/util/print.h"
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

using namespace godot;

namespace {

// void _init_logger() {
// 	if (!SentryOptions::get_singleton()->is_logger_enabled()) {
// 		// If error logger is disabled, don't add it to the scene tree.
// 		sentry::util::print_debug("error logger is disabled in options");
// 		return;
// 	}
// 	if (!SentrySDK::get_singleton()->is_enabled()) {
// 		sentry::util::print_debug("error logger is not started because SDK is not initialized");
// 		return;
// 	}
// 	// Add experimental logger to scene tree.
// 	sentry::util::print_debug("starting error logger");
// 	SceneTree *sml = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
// 	if (sml && sml->get_root()) {
// 		sml->get_root()->add_child(memnew(SentryLogger));
// 	} else {
// 		ERR_FAIL_MSG("Sentry: Internal error: SceneTree is null.");
// 	}
// }

} // unnamed namespace

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
#endif // NATIVE_SDK

		SentryOptions::create_singleton();

		SentrySDK *sentry_singleton = memnew(SentrySDK);
		Engine::get_singleton()->register_singleton("SentrySDK", SentrySDK::get_singleton());

		// callable_mp_static(_init_logger).call_deferred();
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
