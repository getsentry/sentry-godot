#include "auto_metrics_attributes.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/math.hpp>

namespace sentry {

void AutoMetricsAttributes::initialize() {
	_attributes.clear();
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
}

const Dictionary &AutoMetricsAttributes::get_current() {
	SceneTree *tree = SceneTree::get_singleton();
	if (tree == nullptr) {
		return _attributes;
	}

	if (DisplayServer::get_singleton()->get_name() != "headless") {
		Vector2i size = tree->get_root()->get_size();
		_attributes["res.x"] = size.x;
		_attributes["res.y"] = size.y;
	}

	Node *scene = tree->get_current_scene();
	if (scene != nullptr) {
		String path = scene->get_scene_file_path();
		_attributes["scene"] = path.is_empty() ? String(scene->get_name()) : path;
	} else {
		_attributes.erase("scene");
	}
	return _attributes;
}

} // namespace sentry
