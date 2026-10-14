#pragma once

#include "sentry/util/lifetime.h"

#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

namespace sentry {

class AutoMetricsAttributes {
	LIFETIME_OBSERVABLE();

private:
	Dictionary _attributes;
	ObjectID _scene_id;

	void _update_resolution();
	void _update_scene();

public:
	void setup();
	void teardown();
	const Dictionary &get_current();
};

} // namespace sentry
