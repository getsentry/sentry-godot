#pragma once

#include "godot_cpp/classes/node.hpp"

using namespace godot;

namespace sentry::native {

class AppHangHeartbeat : public Node {
	GDCLASS(AppHangHeartbeat, Node)
protected:
	static void _bind_methods() {}

public:
	virtual void _process(double p_delta) override;

	AppHangHeartbeat();
	~AppHangHeartbeat();
};

} //namespace sentry::native
