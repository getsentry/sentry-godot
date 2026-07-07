#pragma once

#include <godot_cpp/classes/node.hpp>

using namespace godot;

namespace sentry::engine_lifecycle {

class SentrySceneTreeWatcher : public Node {
	GDCLASS(SentrySceneTreeWatcher, Node);

public:
	using ShutdownCallback = void (*)();

private:
	ShutdownCallback _shutdown_callback = nullptr;

protected:
	static void _bind_methods() {}

	void _notification(int p_what);

public:
	void set_shutdown_callback(ShutdownCallback p_callback) { _shutdown_callback = p_callback; }
};

} //namespace sentry::engine_lifecycle
