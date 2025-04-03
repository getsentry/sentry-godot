#ifndef SENTRY_VIEW_HIERARCHY_H
#define SENTRY_VIEW_HIERARCHY_H

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace sentry {

godot::String build_view_hierarchy_json();

} //namespace sentry

#endif // SENTRY_VIEW_HIERARCHY_H
