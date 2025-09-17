#include "view_hierarchy_optimized.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace {

class FastStringBuilder {
	godot::String buffer;
	char32_t *ptrw = nullptr;
	size_t sz = 0;
	size_t max_sz = 0;

private:
	inline void _ensure_size(size_t p_expected) {
		if (p_expected > max_sz) {
			max_sz = max_sz * 2;
			buffer.resize(max_sz);
			ptrw = buffer.ptrw();
		}
	}

public:
	FastStringBuilder(size_t p_estimate = 4096) {
		max_sz = p_estimate;
		buffer.resize(max_sz);
		ptrw = buffer.ptrw();
	}

	void append(const char *p_cstr) {
		const size_t length = strlen(p_cstr);
		_ensure_size(sz + length + 1);

		const char *src = p_cstr;
		const char *end = p_cstr + length;

		for (; src < end; ++src, ++ptrw) {
			*ptrw = static_cast<uint8_t>(*src);
		}

		sz += length;
	}

	void append(const String &p_str) {
		const size_t length = p_str.length();
		_ensure_size(sz + length + 1);

		memcpy(ptrw, p_str.ptr(), length * sizeof(char32_t));
		ptrw += length;
		sz += length;
	}

	bool ends_with(const char *p_suffix) {
		if (sz == 0) {
			return false;
		}

		int suffix_len = strlen(p_suffix);
		if (suffix_len > sz) {
			return false;
		}

		// Compare from the end backwards
		const char32_t *end_ptr = ptrw - suffix_len;
		for (int i = 0; i < suffix_len; ++i) {
			if (end_ptr[i] != static_cast<char32_t>(static_cast<uint8_t>(p_suffix[i]))) {
				return false;
			}
		}

		return true;
	}

	size_t get_length() {
		return sz + 1;
	}

	String to_godot_string() {
		*ptrw = 0;
		max_sz = sz + 1;
		buffer.resize(max_sz);
		ptrw = buffer.ptrw();
		return buffer;
	}
};

inline void _start_name_value_pair_new(FastStringBuilder &p_builder, const char *p_name, const String &p_value) {
	p_builder.append("\"");
	p_builder.append(p_name);
	p_builder.append("\":\"");
	p_builder.append(p_value);
	p_builder.append("\"");
}

inline void _next_name_value_pair_new(FastStringBuilder &p_builder, const char *p_name, const String &p_value) {
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

	FastStringBuilder builder{ size_t(estimated_length * 0.2) }; // generously overestimate

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
	estimated_length = MAX(estimated_length, size_t(builder.get_length() * 0.2));

	return builder.to_godot_string();
}

} //namespace sentry
