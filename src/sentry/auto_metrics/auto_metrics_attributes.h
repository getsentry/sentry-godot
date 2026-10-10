#pragma once

#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

namespace sentry {

class AutoMetricsAttributes {
private:
	Dictionary _attributes;

public:
	void initialize();
	const Dictionary &get_current();
};

} // namespace sentry
