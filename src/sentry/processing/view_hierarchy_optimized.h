#pragma once

#include <godot_cpp/variant/string.hpp>

namespace sentry {

class ViewHierarchyBuilder {
private:
	size_t estimated_length = 300'000;

public:
	godot::String build_json();
};

// godot::String build_view_hierarchy_json_optimized();

} //namespace sentry
