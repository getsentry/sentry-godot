#include "view_hierarchy.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

namespace {

Dictionary _build_view_hierarchy_recursive(Node *p_node) {
	if (p_node == nullptr) {
		return Dictionary();
	}

	Dictionary dict;
	dict["identifier"] = p_node->get_name();
	dict["type"] = p_node->get_class();

	String scene_path = p_node->get_scene_file_path();
	if (!scene_path.is_empty()) {
		dict["scene"] = scene_path;
	}

	Ref<Script> scr = p_node->get_script();
	dict["script"] = scr.is_valid() ? scr->get_path() : String();

	Array children;
	children.resize(p_node->get_child_count());
	dict["children"] = children;
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Dictionary child_dict = _build_view_hierarchy_recursive(p_node->get_child(i));
		children[i] = child_dict;
	}
	return dict;
}

} // unnamed namespace
namespace sentry {

String build_view_hierarchy_json() {
	SceneTree *sml = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	ERR_FAIL_NULL_V(sml, String());
	Dictionary root_window = _build_view_hierarchy_recursive(sml->get_root());

	Array windows = Array();
	windows.append(root_window);

	Dictionary view_hierarchy;
	view_hierarchy["rendering_system"] = "Godot";
	view_hierarchy["windows"] = windows;

	return JSON::stringify(view_hierarchy);
}

} //namespace sentry
