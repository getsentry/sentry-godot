#include "sentry_logger.h"
#include "sentry_options.h"
#include "sentry_sdk.h"
#include "sentry_user.h"

#include <sentry.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

namespace {

void _init_logger() {
	if (!SentryOptions::get_singleton()->is_error_logger_enabled()) {
		// If error logger is disabled, don't add it to the scene tree.
		return;
	}
	// Add experimental logger to scene tree.
	SentryLogger *logger = memnew(SentryLogger);
	SceneTree *sml = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	if (sml) {
		sml->get_root()->add_child(logger);
	} else {
		ERR_FAIL_MSG("Sentry: Internal error: SceneTree is null.");
	}
}

} // unnamed namespace

void initialize_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
	} else if (p_level == godot::MODULE_INITIALIZATION_LEVEL_SERVERS) {
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// Note: Godot singletons are only available at higher initialization levels.
		SentryOptions *options = new SentryOptions();

		if (!Engine::get_singleton()->is_editor_hint()) {
			GDREGISTER_INTERNAL_CLASS(SentryLogger);
			callable_mp_static(_init_logger).call_deferred();
		}

		GDREGISTER_CLASS(SentryUser);
		GDREGISTER_CLASS(SentrySDK);
		SentrySDK *sentry_singleton = memnew(SentrySDK);
		Engine::get_singleton()->register_singleton("SentrySDK", SentrySDK::get_singleton());

		// TODO: Restore context init!
		// Some singletons are not available at this point.
		callable_mp(sentry_singleton, &SentrySDK::add_device_context).call_deferred();
		callable_mp(sentry_singleton, &SentrySDK::add_app_context).call_deferred();
		// callable_mp(sentry_singleton, &SentrySDK::add_gpu_context).call_deferred();
		// callable_mp(sentry_singleton, &SentrySDK::add_culture_context).call_deferred();
		// callable_mp(sentry_singleton, &SentrySDK::add_display_context).call_deferred();
		// callable_mp(sentry_singleton, &SentrySDK::add_engine_context).call_deferred();
		// callable_mp(sentry_singleton, &SentrySDK::add_environment_context).call_deferred();
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
	}
#endif // TOOLS_ENABLED
}

void uninitialize_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
		memdelete(SentrySDK::get_singleton());
		delete SentryOptions::get_singleton();
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
