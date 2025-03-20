#include "view_hierarchy.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace {

inline void _start_name_value_pair(String &p_arr, const String &p_name, const String &p_value) {
	p_arr += "\"" + p_name + "\":\"" + p_value + "\"";
}

inline void _next_name_value_pair(String &p_arr, const String &p_name, const String &p_value) {
	p_arr += ",\"" + p_name + "\":\"" + p_value + "\"";
}

inline void _next_name_value_pair_raw(String &p_arr, const String &p_name, const String &p_value) {
	p_arr += ",\"" + p_name + "\":" + p_value;
}

} // unnamed namespace

namespace sentry {

String build_view_hierarchy_json(const Vector<StringName> &p_extra_properties) {
	SceneTree *sml = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	ERR_FAIL_NULL_V(sml, String());

	String json = R"({"rendering_system":"Godot","windows":[)";
	List<Node *> stack;
	List<int> hierarchy;

	if (sml->get_root()) {
		stack.push_back(sml->get_root());
	}

	while (!stack.is_empty()) {
		if (json.ends_with("}")) {
			json += ",{";
		} else {
			json += "{";
		}

		Node *node = stack.back()->get();
		stack.pop_back();

		_start_name_value_pair(json, "identifier", node->get_name());
		_next_name_value_pair(json, "type", node->get_class());

		String scene_path = node->get_scene_file_path();
		if (!scene_path.is_empty()) {
			_next_name_value_pair(json, "scene", scene_path);
		}

		const Ref<Script> &scr = node->get_script();
		if (scr.is_valid()) {
			_next_name_value_pair(json, "script", scr.is_valid() ? scr->get_path() : String());
		}

		for (const StringName &prop_name : p_extra_properties) {
			Variant value = node->get(prop_name);
			Variant::Type value_type = value.get_type();
			if (value_type != Variant::NIL && value_type < Variant::DICTIONARY) {
				_next_name_value_pair_raw(json, prop_name, JSON::stringify(value));
			} else if (value_type >= Variant::DICTIONARY) {
				_next_name_value_pair(json, prop_name, value.operator String().json_escape());
			}
		}

		if (node->get_child_count()) {
			json += R"(,"children":[)";
			for (int i = node->get_child_count() - 1; i >= 0; i--) {
				stack.push_back(node->get_child(i));
			}
			hierarchy.push_back(node->get_child_count());
		} else {
			json += "}";
			while (!hierarchy.is_empty() && (--hierarchy.back()->get()) == 0) {
				json += "]}";
				hierarchy.pop_back();
			}
		}
	}

	json += "]}";
	return json;
}

} //namespace sentry
