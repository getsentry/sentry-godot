#include "view_hierarchy_builder.h"

#include "sentry/util/fast_string_builder.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace {

inline void _start_name_value_pair_new(sentry::util::FastStringBuilder &p_builder, const char *p_name, const String &p_value) {
	p_builder.append("\"");
	p_builder.append(p_name);
	p_builder.append("\":\"");
	p_builder.append(p_value);
	p_builder.append("\"");
}

inline void _next_name_value_pair_new(sentry::util::FastStringBuilder &p_builder, const char *p_name, const String &p_value) {
	p_builder.append(",\"");
	p_builder.append(p_name);
	p_builder.append("\":\"");
	p_builder.append(p_value);
	p_builder.append("\"");
}

} // unnamed namespace

namespace sentry {

String ViewHierarchyBuilder::build_json() {
	SceneTree *sml = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	ERR_FAIL_NULL_V(sml, String());

	sentry::util::FastStringBuilder builder{ size_t(estimated_length * 0.2) }; // generously overestimate

	builder.append(R"({"rendering_system":"Godot","windows":[)");

	List<Node *> stack;
	List<int> hierarchy;

	if (sml->get_root()) {
		stack.push_back(sml->get_root());
	}

	while (!stack.is_empty()) {
		if (builder.ends_with("}")) {
			builder.append(",{");
		} else {
			builder.append("{");
		}

		Node *node = stack.back()->get();
		stack.pop_back();

		_start_name_value_pair_new(builder, "name", node->get_name());
		_next_name_value_pair_new(builder, "class", node->get_class());

		String scene_path = node->get_scene_file_path();
		if (!scene_path.is_empty()) {
			_next_name_value_pair_new(builder, "scene", scene_path);
		}

		const Ref<Script> &scr = node->get_script();
		if (scr.is_valid()) {
			_next_name_value_pair_new(builder, "script", scr.is_valid() ? scr->get_path() : String());
		}

		if (node->get_child_count()) {
			builder.append(",\"children\":[");
			for (int i = node->get_child_count() - 1; i >= 0; i--) {
				stack.push_back(node->get_child(i));
			}
			hierarchy.push_back(node->get_child_count());
		} else {
			builder.append("}");
			while (!hierarchy.is_empty() && (--hierarchy.back()->get()) == 0) {
				builder.append("]}");
				hierarchy.pop_back();
			}
		}
	}

	builder.append("]}");

	// Update estimate
	estimated_length = MAX(estimated_length, size_t(builder.get_length() * 1.2));

	return builder.to_string();
}

} //namespace sentry
