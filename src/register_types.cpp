
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
	}
#endif // TOOLS_ENABLED
}

void uninitialize_module(ModuleInitializationLevel p_level) {
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
