#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace sentry::engine_lifecycle {

class SentrySceneTreeWatcher : public Node {
	GDCLASS(SentrySceneTreeWatcher, Node);

private:
	Callable _shutdown_callback;

protected:
	static void _bind_methods() {}

	void _notification(int p_what);

public:
	void set_shutdown_callback(const Callable &p_callback) { _shutdown_callback = p_callback; }
};

} //namespace sentry::engine_lifecycle
