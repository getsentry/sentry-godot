#include "engine_lifecycle.h"

#include "sentry/engine_lifecycle/sentry_scene_tree_watcher.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <atomic>

using namespace godot;
using namespace sentry::engine_lifecycle;

namespace {

// Tracks whether engine singletons are fully initialized and safe to access.
std::atomic<bool> _singletons_ready{ false };

// Shutdown subscribers, notified while script runtime is still alive.
LocalVector<Callable> _shutdown_callbacks;

// Whether the lifecycle watch has already been started.
bool _watch_started = false;

void _scene_tree_shutting_down() {
	for (const Callable &callback : _shutdown_callbacks) {
		callback.call();
	}
}

void _add_scene_tree_watcher() {
	SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	ERR_FAIL_NULL_MSG(tree, "Sentry: Failed to initialize engine lifecycle tracking - SceneTree is unavailable.");
	Window *root = tree->get_root();
	ERR_FAIL_NULL_MSG(root, "Sentry: Failed to initialize engine lifecycle tracking - root window is unavailable.");

	SentrySceneTreeWatcher *watcher = memnew(SentrySceneTreeWatcher);
	watcher->set_shutdown_callback(callable_mp_static(&_scene_tree_shutting_down));
	// Add at the front so it is torn down after other scene tree nodes.
	root->add_child(watcher, false, Node::INTERNAL_MODE_FRONT);
}

} // unnamed namespace

namespace sentry::engine_lifecycle {

void start_lifecycle_watch() {
	if (_watch_started) {
		return;
	}
	// Deferred: the engine isn't fully up yet.
	callable_mp_static(&_add_scene_tree_watcher).call_deferred();
	_watch_started = true;
}

void mark_engine_singletons_as_ready() {
	_singletons_ready.store(true, std::memory_order_release);
}

bool are_engine_singletons_ready() {
	return _singletons_ready.load(std::memory_order_acquire);
}

void add_shutdown_callback(const Callable &p_callback) {
	_shutdown_callbacks.push_back(p_callback);
}

void remove_shutdown_callback(const Callable &p_callback) {
	_shutdown_callbacks.erase(p_callback);
}

} // namespace sentry::engine_lifecycle
