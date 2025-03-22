#include "screenshot.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/window.hpp>

using namespace godot;

namespace sentry::util {

PackedByteArray take_screenshot() {
	SceneTree *sml = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
	ERR_FAIL_NULL_V_MSG(sml, PackedByteArray(), "Sentry: Failed to capture screenshot - couldn't get scene tree.");

	Window *main_window = sml->get_root();
	ERR_FAIL_NULL_V_MSG(main_window, PackedByteArray(), "Sentry: Failed to capture screenshot - couldn't get main window.");

	Ref<ViewportTexture> tex = main_window->get_texture();
	return tex->get_image()->save_jpg_to_buffer();
}

} //namespace sentry::util
