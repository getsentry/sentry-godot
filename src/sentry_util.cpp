#include "sentry_util.h"
#include "sentry/native/native_util.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

String SentryUtil::get_screen_orientation_string(int32_t p_screen) {
	ERR_FAIL_NULL_V(DisplayServer::get_singleton(), "");
	switch (DisplayServer::get_singleton()->screen_get_orientation(p_screen)) {
		case DisplayServer::SCREEN_LANDSCAPE: {
			return "Landscape";
		} break;
		case DisplayServer::SCREEN_PORTRAIT: {
			return "Portrait";
		} break;
		case DisplayServer::SCREEN_REVERSE_LANDSCAPE: {
			return "Landscape (reverse)";
		} break;
		case DisplayServer::SCREEN_REVERSE_PORTRAIT: {
			return "Portrait (reverse)";
		} break;
		case DisplayServer::SCREEN_SENSOR_LANDSCAPE: {
			return "Landscape (defined by sensor)";
		} break;
		case DisplayServer::SCREEN_SENSOR_PORTRAIT: {
			return "Portrait (defined by sensor)";
		} break;
		case DisplayServer::SCREEN_SENSOR: {
			return "Defined by sensor";
		} break;
		default: {
			return "";
		} break;
	}
}
