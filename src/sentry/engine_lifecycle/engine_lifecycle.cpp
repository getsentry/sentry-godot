#include "engine_lifecycle.h"

#include "sentry/engine_lifecycle/sentry_scene_tree_watcher.h"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <atomic>
#include <utility>

using namespace godot;
using namespace sentry::engine_lifecycle;

namespace {

// Tracks whether engine singletons are fully initialized and safe to access.
std::atomic<bool> _singletons_ready{ false };

// Shutdown subscribers, notified while script runtime is still alive.
LocalVector<Callable> _shutdown_callbacks;

// Termination subscribers, notified as the extension is torn down.
LocalVector<Callable> _module_termination_callbacks;

// Whether the lifecycle watch has already been started.
bool _watch_started = false;
bool _shutdown_started = false;

void _add_scene_tree_watcher() {
	SceneTree *tree = SceneTree::get_singleton();
	ERR_FAIL_NULL_MSG(tree, "Sentry: Failed to initialize engine lifecycle tracking - SceneTree is unavailable.");
	Window *root = tree->get_root();
	ERR_FAIL_NULL_MSG(root, "Sentry: Failed to initialize engine lifecycle tracking - root window is unavailable.");

	SentrySceneTreeWatcher *watcher = memnew(SentrySceneTreeWatcher);
	watcher->set_shutdown_callback(callable_mp_static(&notify_engine_shutting_down));
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
	ERR_FAIL_COND_MSG(_shutdown_started, "Sentry: Cannot register a shutdown callback after shutdown has started.");
	_shutdown_callbacks.push_back(p_callback);
}

void remove_shutdown_callback(const Callable &p_callback) {
	_shutdown_callbacks.erase(p_callback);
}

void notify_engine_shutting_down() {
	if (_shutdown_started) {
		return;
	}
	_shutdown_started = true;
	LocalVector<Callable> callbacks = std::move(_shutdown_callbacks);
	for (const Callable &callback : callbacks) {
		callback.call();
	}
}

Error attach_predelete_script(Object *p_object, const Callable &p_callback) {
	if (!p_object || !p_callback.is_valid()) {
		return ERR_INVALID_PARAMETER;
	}
	if (p_object->get_script().get_type() != Variant::NIL) {
		return ERR_ALREADY_IN_USE;
	}

	Ref<Script> script = ClassDB::instantiate("GDScript");
	if (script.is_null()) {
		return ERR_UNAVAILABLE;
	}
	script->set_source_code(R"(@tool
extends Object
var shutdown_callback: Callable

func _notification(what: int) -> void:
	if what == NOTIFICATION_PREDELETE and shutdown_callback.is_valid():
		shutdown_callback.call()
)");
	Error error = script->reload();
	if (error != OK) {
		return error;
	}
	p_object->set_script(script);
	if (!script->instance_has(p_object)) {
		return ERR_CANT_CREATE;
	}
	p_object->set("shutdown_callback", p_callback);
	return OK;
}

void add_module_termination_callback(const Callable &p_callback) {
	_module_termination_callbacks.push_back(p_callback);
}

void notify_module_terminating() {
	for (uint32_t i = _module_termination_callbacks.size(); i > 0; --i) {
		_module_termination_callbacks[i - 1].call();
	}

	_module_termination_callbacks.clear();
	_shutdown_callbacks.clear();
}

} // namespace sentry::engine_lifecycle
