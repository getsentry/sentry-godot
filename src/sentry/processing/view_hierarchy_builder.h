#pragma once

#include <godot_cpp/variant/string.hpp>

namespace sentry {

class ViewHierarchyBuilder {
private:
	size_t estimated_length = 300'000; // set generous initial estimate

public:
	godot::String build_json();
};

} //namespace sentry
