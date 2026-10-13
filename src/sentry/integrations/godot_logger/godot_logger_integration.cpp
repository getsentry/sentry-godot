#include "godot_logger_integration.h"

#include <godot_cpp/classes/os.hpp>

namespace sentry {

bool GodotLoggerIntegration::setup(const Ref<SentryOptions> &p_options) {
	if (!p_options->get_godot_logger()->get_enabled()) {
		return false;
	}

	_logger.instantiate();
	OS::get_singleton()->add_logger(_logger);
	return true;
}

void GodotLoggerIntegration::teardown() {
	if (_logger.is_null()) {
		return;
	}

	OS::get_singleton()->remove_logger(_logger);
	_logger.unref();
}

} //namespace sentry
