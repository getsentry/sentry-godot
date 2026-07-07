#include "engine_lifecycle.h"

#include "sentry/engine_lifecycle/sentry_scene_tree_watcher.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>

#include <atomic>

using namespace godot;
using namespace sentry::engine_lifecycle;

namespace {

// Tracks whether engine singletons are fully initialized and safe to access.
std::atomic<bool> singletons_ready{ false };

// Tracks whether the engine is shutting down.
std::atomic<bool> shutting_down{ false };

void _scene_tree_shutting_down() {
	shutting_down.store(true, std::memory_order_release);
}

void _initialize_scene_tree_watcher() {
	SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	ERR_FAIL_NULL_MSG(tree, "Sentry: Failed to initialize engine lifecycle tracking - SceneTree is unavailable.");
	SentrySceneTreeWatcher *watcher = memnew(SentrySceneTreeWatcher);
	tree->get_root()->add_child(watcher);
	watcher->set_callback(callable_mp_static(&_scene_tree_shutting_down));
}

} // unnamed namespace

namespace sentry::engine_lifecycle {

void mark_engine_singletons_as_ready() {
	singletons_ready.store(true, std::memory_order_release);
	_initialize_scene_tree_watcher();
}

bool are_engine_singletons_ready() {
	return singletons_ready.load(std::memory_order_acquire);
}

bool is_shutting_down() {
	return shutting_down.load(std::memory_order_acquire);
}

} // namespace sentry::engine_lifecycle
