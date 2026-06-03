#ifdef TESTS_ENABLED

#include "dotnet_test_support.h"

#include "sentry/dotnet/csharp_interop.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>

using godot::Ref;
using godot::RefCounted;
using godot::ResourceLoader;
using godot::Script;

namespace {

constexpr const char *HARNESS_PATH = "res://test/dotnet/DotnetTestHarness.cs";

Ref<RefCounted> s_harness;
bool s_tried_loading_harness = false;

void _ensure_harness_loaded() {
	if (s_tried_loading_harness) {
		return;
	}
	s_tried_loading_harness = true;

	if (!sentry::dotnet::godot_supports_dotnet()) {
		return;
	}

	Ref<Script> script = ResourceLoader::get_singleton()->load(HARNESS_PATH);
	if (script.is_null()) {
		return;
	}

	s_harness = script->call("new");
}

} // namespace

namespace sentry::tests {

godot::Object *get_dotnet_harness() {
	_ensure_harness_loaded();
	return s_harness.ptr();
}

} // namespace sentry::tests

#endif // TESTS_ENABLED
