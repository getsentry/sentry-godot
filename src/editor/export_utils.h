#pragma once

#include <godot_cpp/classes/editor_export_platform.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry::editor {

// Creates a dictionary with a fake export check option for Sentry export plugins.
// This is needed because Godot's export dialog only shows bottom-bar warnings for
// plugin-owned options, so a fake option allows us to display warnings for
// Godot Engine-owned export options through that code path.
// Option is hidden from view, so users don't actually see it in the UI.
inline Dictionary make_hidden_export_check_option() {
	Dictionary option;
	option["option"] = Dictionary(PropertyInfo(
			Variant::BOOL,
			"sentry/_export_check",
			PROPERTY_HINT_NONE,
			String(),
			PROPERTY_USAGE_NONE));
	option["default_value"] = true;
	return option;
}

// Checks if this option is either the builtin one we care about or our hidden sentry option.
// See make_hidden_export_check_option().
inline bool is_builtin_option_or_hidden_export_option(const String &p_option, const String &p_builtin_option) {
	return p_option == p_builtin_option || p_option == "sentry/_export_check";
}

} //namespace sentry::editor
