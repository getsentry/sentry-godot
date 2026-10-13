#include "auto_metrics_attributes.h"

#include "sentry/util/callable_cpp.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/math.hpp>

namespace sentry {

void AutoMetricsAttributes::_update_resolution() {
	if (DisplayServer::get_singleton()->get_name() != "headless") {
		Vector2i size = DisplayServer::get_singleton()->window_get_size();
		_attributes["res.x"] = size.x;
		_attributes["res.y"] = size.y;
	}
}

void AutoMetricsAttributes::_update_scene() {
	SceneTree *tree = SceneTree::get_singleton();
	if (tree) {
		// SceneTree::set_current_scene() unfortunately doesn't emit scene_changed,
		// so we poll it just in case. Quick scene id check keeps it fast enough.
		Node *scene = tree->get_current_scene();
		ObjectID scene_id = scene != nullptr ? ObjectID(scene->get_instance_id()) : ObjectID();
		if (scene_id != _scene_id) {
			_scene_id = scene_id;
			if (scene != nullptr) {
				String path = scene->get_scene_file_path();
				_attributes["scene"] = path.is_empty() ? String(scene->get_name()) : path;
			} else {
				_attributes.erase("scene");
			}
		}
	}
}

void AutoMetricsAttributes::setup() {
	_attributes.clear();
	_scene_id = ObjectID();
	if (DisplayServer::get_singleton()->get_name() != "headless") {
		String gpu_name = RenderingServer::get_singleton()->get_video_adapter_name();
		if (!gpu_name.is_empty()) {
			_attributes["gpu.name"] = gpu_name;
		}
	}

	int32_t cpu_cores = OS::get_singleton()->get_processor_count();
	if (cpu_cores > 0) {
		_attributes["cpu.cores"] = cpu_cores;
	}

	Dictionary memory_info = OS::get_singleton()->get_memory_info();
	int64_t physical_memory = memory_info.get("physical", -1);
	if (physical_memory > 0) {
		// Memory is reported as physical minus reserved by kernel and drivers.
		_attributes["ram.gb"] = Math::snapped(physical_memory / (1024.0 * 1024.0 * 1024.0), 0.01);
	}

	SceneTree *tree = SceneTree::get_singleton();
	Window *window = tree != nullptr ? tree->get_root() : nullptr;
	if (window) {
		window->connect("size_changed", callable_cpp(this, &AutoMetricsAttributes::_update_resolution));
	}
	_update_resolution();
}

void AutoMetricsAttributes::teardown() {
	SceneTree *tree = SceneTree::get_singleton();
	Window *window = tree != nullptr ? tree->get_root() : nullptr;
	if (window) {
		Callable callback = callable_cpp(this, &AutoMetricsAttributes::_update_resolution);
		if (window->is_connected("size_changed", callback)) {
			window->disconnect("size_changed", callback);
		}
	}
}

const Dictionary &AutoMetricsAttributes::get_current() {
	_update_scene();
	return _attributes;
}

} // namespace sentry
