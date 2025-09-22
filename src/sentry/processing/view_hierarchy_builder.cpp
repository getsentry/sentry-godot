#include "view_hierarchy_builder.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace {

inline void _start_name_value_pair_new(sentry::util::UTF8Buffer &p_builder, const char *p_name, const String &p_value) {
	p_builder.append("\"");
	p_builder.append(p_name);
	p_builder.append("\":\"");
	p_builder.append(p_value);
	p_builder.append("\"");
}

inline void _next_name_value_pair_new(sentry::util::UTF8Buffer &p_builder, const char *p_name, const String &p_value) {
	p_builder.append(",\"");
	p_builder.append(p_name);
	p_builder.append("\":\"");
	p_builder.append(p_value);
	p_builder.append("\"");
}

} // unnamed namespace

namespace sentry {

sentry::util::UTF8Buffer ViewHierarchyBuilder::build_json() {
	SceneTree *sml = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	ERR_FAIL_NULL_V(sml, ::sentry::util::UTF8Buffer(0));

	sentry::util::UTF8Buffer buffer{ size_t(estimated_buffer_size) };

	buffer.append(R"({"rendering_system":"Godot","windows":[)");

	List<Node *> stack;
	List<int> hierarchy;

	if (sml->get_root()) {
		stack.push_back(sml->get_root());
	}

	while (!stack.is_empty()) {
		if (buffer.ends_with("}")) {
			buffer.append(",{");
		} else {
			buffer.append("{");
		}

		Node *node = stack.back()->get();
		stack.pop_back();

		_start_name_value_pair_new(buffer, "name", node->get_name());
		_next_name_value_pair_new(buffer, "class", node->get_class());

		String scene_path = node->get_scene_file_path();
		if (!scene_path.is_empty()) {
			_next_name_value_pair_new(buffer, "scene", scene_path);
		}

		const Ref<Script> &scr = node->get_script();
		if (scr.is_valid()) {
			_next_name_value_pair_new(buffer, "script", scr.is_valid() ? scr->get_path() : String());
		}

		if (node->get_child_count()) {
			buffer.append(",\"children\":[");
			for (int i = node->get_child_count() - 1; i >= 0; i--) {
				stack.push_back(node->get_child(i));
			}
			hierarchy.push_back(node->get_child_count());
		} else {
			buffer.append("}");
			while (!hierarchy.is_empty() && (--hierarchy.back()->get()) == 0) {
				buffer.append("]}");
				hierarchy.pop_back();
			}
		}
	}

	buffer.append("]}");

	// Update estimate
	estimated_buffer_size = MAX(estimated_buffer_size, buffer.get_capacity());

	return buffer;
}

} //namespace sentry
