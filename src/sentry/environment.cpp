#include "environment.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>

namespace sentry::environment {

String detect_godot_environment() {
	ERR_FAIL_NULL_V(Engine::get_singleton(), "production");
	ERR_FAIL_NULL_V(OS::get_singleton(), "production");

	// We need to have either "dev" or "debug" in the name to prioritize dev environments:
	// https://develop.sentry.dev/application-architecture/dynamic-sampling/fidelity-and-biases/#prioritize-dev-environments
	if (OS::get_singleton()->has_feature("dedicated_server")) {
		return "dedicated_server";
	} else if (Engine::get_singleton()->is_editor_hint()) {
		return "editor_dev";
	} else if (OS::get_singleton()->has_feature("editor")) {
		return "editor_dev_run";
	} else if (OS::get_singleton()->is_debug_build()) {
		return "export_debug";
	} else {
		return "export_release";
	}
}

} //namespace sentry::environment
