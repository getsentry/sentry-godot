#include "environment.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>

namespace sentry::environment {

CharString get_environment() {
	ERR_FAIL_NULL_V(Engine::get_singleton(), "production");
	ERR_FAIL_NULL_V(OS::get_singleton(), "production");

	if (OS::get_singleton()->has_feature("dedicated_server")) {
		return "dedicated_server";
	} else if (Engine::get_singleton()->is_editor_hint()) {
		return "editor";
	} else if (OS::get_singleton()->has_feature("editor")) {
		return "editor-run";
	} else if (OS::get_singleton()->is_debug_build()) {
		return "export-debug";
	} else {
		return "export-release";
	}
}

} //namespace sentry::environment
