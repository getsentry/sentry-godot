#pragma once

#include <godot_cpp/classes/node.hpp>

using namespace godot;

namespace sentry::engine_lifecycle {

class SentrySceneTreeWatcher : public Node {
	GDCLASS(SentrySceneTreeWatcher, Node);

private:
	Callable callback;

protected:
	static void _bind_methods() {}

	void _notification(int p_what);

public:
	void set_callback(const Callable &p_callback) { callback = p_callback; }
};

} //namespace sentry::engine_lifecycle
