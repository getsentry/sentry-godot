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

// Shutdown subscribers, notified while script runtime is still alive.
LocalVector<sentry::util::Callback<>> _shutdown_callbacks;

// Whether the scene tree watcher has already been created.
bool _watcher_added = false;

void _scene_tree_shutting_down() {
	for (const sentry::util::Callback<> &callback : _shutdown_callbacks) {
		callback.call();
	}
}

void _initialize_scene_tree_watcher() {
	if (_watcher_added) {
		return;
	}
	SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	ERR_FAIL_NULL_MSG(tree, "Sentry: Failed to initialize engine lifecycle tracking - SceneTree is unavailable.");
	SentrySceneTreeWatcher *watcher = memnew(SentrySceneTreeWatcher);
	watcher->set_shutdown_callback(sentry::util::Callback<>::bind<&_scene_tree_shutting_down>());
	// Add at the front so it is torn down after other scene tree nodes.
	// Deferred because adding directly can fail while the root is busy
	// (e.g, when init() is called from a node's _ready()).
	tree->get_root()->call_deferred("add_child", watcher, false, Node::INTERNAL_MODE_FRONT);
	_watcher_added = true;
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

void add_shutdown_callback(util::Callback<> p_callback) {
	_shutdown_callbacks.push_back(p_callback);
}

void remove_shutdown_callback(util::Callback<> p_callback) {
	_shutdown_callbacks.erase(p_callback);
}

} // namespace sentry::engine_lifecycle
