#ifndef SENTRY_VIEW_HIERARCHY_H
#define SENTRY_VIEW_HIERARCHY_H

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace sentry {

godot::String build_view_hierarchy_json();

// Individual implementations for benchmarking
godot::String build_view_hierarchy_json();
godot::String build_view_hierarchy_json_optimized();

// Performance measurement functions (debug builds only) - defined in view_hierarchy_benchmark.cpp
void benchmark_view_hierarchy_performance();

} //namespace sentry

#endif // SENTRY_VIEW_HIERARCHY_H
