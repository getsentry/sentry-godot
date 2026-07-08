#include "engine_lifecycle.h"

#include "sentry/engine_lifecycle/sentry_scene_tree_watcher.h"
#include "sentry/util/callback.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/templates/local_vector.hpp>

#include <atomic>

using namespace godot;
using namespace sentry::engine_lifecycle;

namespace {

// Tracks whether engine singletons are fully initialized and safe to access.
std::atomic<bool> _singletons_ready{ false };

// Tracks whether the engine is shutting down.
std::atomic<bool> _shutting_down{ false };

// Shutdown subscribers, notified while script runtime is still alive.
LocalVector<sentry::util::Callback<>> _shutdown_callbacks;

void _scene_tree_shutting_down() {
	_shutting_down.store(true, std::memory_order_release);
	for (const sentry::util::Callback<> &callback : _shutdown_callbacks) {
		callback.call();
	}
}

void _initialize_scene_tree_watcher() {
	SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	ERR_FAIL_NULL_MSG(tree, "Sentry: Failed to initialize engine lifecycle tracking - SceneTree is unavailable.");
	SentrySceneTreeWatcher *watcher = memnew(SentrySceneTreeWatcher);
	watcher->set_shutdown_callback(&_scene_tree_shutting_down);
	// Add at the front so it is torn down after other scene tree nodes.
	// TODO: figure out how to avoid deferring, in case engine is started for one frame (CLI commands etc)
	tree->get_root()->call_deferred("add_child", watcher, false, Node::INTERNAL_MODE_FRONT);
}

} // unnamed namespace

namespace sentry::engine_lifecycle {

void mark_engine_singletons_as_ready() {
	_singletons_ready.store(true, std::memory_order_release);
	_initialize_scene_tree_watcher();
}

bool are_engine_singletons_ready() {
	return _singletons_ready.load(std::memory_order_acquire);
}

bool is_shutting_down() {
	return _shutting_down.load(std::memory_order_acquire);
}

void add_shutdown_callback(util::Callback<> p_callback) {
	_shutdown_callbacks.push_back(p_callback);
}

void remove_shutdown_callback(util::Callback<> p_callback) {
	_shutdown_callbacks.erase(p_callback);
}

} // namespace sentry::engine_lifecycle
