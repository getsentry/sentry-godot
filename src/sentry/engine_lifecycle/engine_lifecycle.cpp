#include "engine_lifecycle.h"

#include "sentry/engine_lifecycle/sentry_scene_tree_watcher.h"
#include "sentry/logging/print.h"
#include "sentry/util/callback.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
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

// Number of event-processing sections currently in flight (across all threads).
std::atomic<int> _in_flight_processing{ 0 };

// Maximum time to wait for in-flight processing to finish during shutdown.
constexpr uint64_t DRAIN_TIMEOUT_MS = 2000;

// Shutdown subscribers, notified while script runtime is still alive.
LocalVector<sentry::util::Callback<>> _shutdown_callbacks;

// Block the main thread until no processing section is in flight (bounded by a timeout).
void _drain_in_flight_processing() {
	if (_in_flight_processing.load(std::memory_order_seq_cst) == 0) {
		return;
	}
	const uint64_t deadline = Time::get_singleton()->get_ticks_msec() + DRAIN_TIMEOUT_MS;
	while (_in_flight_processing.load(std::memory_order_seq_cst) > 0) {
		if (Time::get_singleton()->get_ticks_msec() >= deadline) {
			// Avoid logger: we are mid-shutdown and must not re-enter capture.
			sentry::logging::print_no_logger(sentry::LEVEL_WARNING, "Timed out draining in-flight event processing during shutdown - proceeding anyway.");
			break;
		}
		OS::get_singleton()->delay_usec(1000);
	}
}

void _scene_tree_shutting_down() {
	_shutting_down.store(true, std::memory_order_seq_cst);
	_drain_in_flight_processing();
	for (const sentry::util::Callback<> &callback : _shutdown_callbacks) {
		callback.call();
	}
}

void _initialize_scene_tree_watcher() {
	SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	ERR_FAIL_NULL_MSG(tree, "Sentry: Failed to initialize engine lifecycle tracking - SceneTree is unavailable.");
	SentrySceneTreeWatcher *watcher = memnew(SentrySceneTreeWatcher);
	watcher->set_shutdown_callback(sentry::util::Callback<>::bind<&_scene_tree_shutting_down>());
	// Add at the front so it is torn down after other scene tree nodes.
	// Deferred because adding directly can fail while the root is busy
	// (e.g, when init() is called from a node's _ready()).
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
	return _shutting_down.load(std::memory_order_seq_cst);
}

ProcessingSection::ProcessingSection() {
	_in_flight_processing.fetch_add(1, std::memory_order_seq_cst);
}

ProcessingSection::~ProcessingSection() {
	_in_flight_processing.fetch_sub(1, std::memory_order_seq_cst);
}

void add_shutdown_callback(util::Callback<> p_callback) {
	_shutdown_callbacks.push_back(p_callback);
}

void remove_shutdown_callback(util::Callback<> p_callback) {
	_shutdown_callbacks.erase(p_callback);
}

} // namespace sentry::engine_lifecycle
