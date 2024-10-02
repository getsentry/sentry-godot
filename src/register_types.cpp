#include "sentry_settings.h"
#include "sentry_singleton.h"

#include <sentry.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
	} else if (p_level == godot::MODULE_INITIALIZATION_LEVEL_SERVERS) {
	} else if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// Note: Godot singletons are only available at higher initialization levels.
		SentrySettings *settings = new SentrySettings();
		GDREGISTER_CLASS(Sentry);
		Sentry *sentry_singleton = memnew(Sentry);
		Engine::get_singleton()->register_singleton("Sentry", Sentry::get_singleton());

		// Some singletons are not available at this point.
		callable_mp(sentry_singleton, &Sentry::add_gpu_context).call_deferred();
		callable_mp(sentry_singleton, &Sentry::add_display_context).call_deferred();
		callable_mp(sentry_singleton, &Sentry::add_engine_context).call_deferred();
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
	}
#endif // TOOLS_ENABLED
}

void uninitialize_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_CORE) {
		memdelete(Sentry::get_singleton());
		delete SentrySettings::get_singleton();
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
