#pragma once

#include "sentry/util/callback.h"

#include <godot_cpp/classes/node.hpp>

using namespace godot;

namespace sentry::engine_lifecycle {

class SentrySceneTreeWatcher : public Node {
	GDCLASS(SentrySceneTreeWatcher, Node);

private:
	sentry::util::Callback<> _shutdown_callback;

protected:
	static void _bind_methods() {}

	void _notification(int p_what);

public:
	void set_shutdown_callback(sentry::util::Callback<> p_callback) { _shutdown_callback = p_callback; }
};

} //namespace sentry::engine_lifecycle
