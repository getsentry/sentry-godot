#include "sentry_singleton.h"

#include "sentry.h"
#include "sentry_settings.h"
#include "sentry_util.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

Sentry *Sentry::singleton = nullptr;

void Sentry::add_gpu_context() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	ERR_FAIL_NULL(OS::get_singleton());

	sentry_value_t gpu_context = sentry_value_new_object();
	sentry_value_set_by_key(gpu_context, "name",
			sentry_value_new_string(RenderingServer::get_singleton()->get_video_adapter_name().utf8()));
	sentry_value_set_by_key(gpu_context, "vendor_name",
			sentry_value_new_string(RenderingServer::get_singleton()->get_video_adapter_vendor().utf8()));
	sentry_value_set_by_key(gpu_context, "version",
			sentry_value_new_string(RenderingServer::get_singleton()->get_video_adapter_api_version().utf8()));

	// Driver info.
	PackedStringArray driver_info = OS::get_singleton()->get_video_adapter_driver_info();
	if (driver_info.size() >= 2) {
		sentry_value_set_by_key(gpu_context, "driver_name", sentry_value_new_string(driver_info[0].utf8()));
		sentry_value_set_by_key(gpu_context, "driver_version", sentry_value_new_string(driver_info[1].utf8()));
	}

	// Device type.
	String device_type = "unknown";
	switch (RenderingServer::get_singleton()->get_video_adapter_type()) {
		case RenderingDevice::DEVICE_TYPE_OTHER: {
			device_type = "Other";
		} break;
		case RenderingDevice::DEVICE_TYPE_INTEGRATED_GPU: {
			device_type = "Integrated GPU";
		} break;
		case RenderingDevice::DEVICE_TYPE_DISCRETE_GPU: {
			device_type = "Discrete GPU";
		} break;
		case RenderingDevice::DEVICE_TYPE_VIRTUAL_GPU: {
			device_type = "Virtual GPU";
		} break;
		case RenderingDevice::DEVICE_TYPE_CPU: {
			device_type = "CPU";
		} break;
		default: {
			device_type = "Unknown";
		} break;
	}
	sentry_value_set_by_key(gpu_context, "device_type", sentry_value_new_string(device_type.utf8()));

	sentry_set_context("gpu", gpu_context);
}

void Sentry::add_display_context() {
	ERR_FAIL_NULL(DisplayServer::get_singleton());

	sentry_value_t display_context = sentry_value_new_object();

	int32_t num_screens = DisplayServer::get_singleton()->get_screen_count();
	sentry_value_set_by_key(display_context, "screen_count",
			sentry_value_new_int32(num_screens));
	sentry_value_set_by_key(display_context, "display_server",
			sentry_value_new_string(DisplayServer::get_singleton()->get_name().utf8()));

	sentry_value_t screen_list = sentry_value_new_list();
	for (int32_t i = 0; i < num_screens; i++) {
		sentry_value_t screen_data = sentry_value_new_object();
		sentry_value_set_by_key(screen_data, "size",
				SentryUtil::variant_to_sentry_value(DisplayServer::get_singleton()->screen_get_size(i)));
		sentry_value_set_by_key(screen_data, "dpi",
				sentry_value_new_int32(DisplayServer::get_singleton()->screen_get_dpi(i)));
		sentry_value_set_by_key(screen_data, "refresh_rate",
				sentry_value_new_int32(DisplayServer::get_singleton()->screen_get_refresh_rate(i)));
		sentry_value_set_by_key(screen_data, "position",
				SentryUtil::variant_to_sentry_value(DisplayServer::get_singleton()->screen_get_position(i)));
		sentry_value_set_by_key(screen_data, "scale_factor",
				sentry_value_new_int32(DisplayServer::get_singleton()->screen_get_scale(i)));
		sentry_value_set_by_key(screen_data, "primary",
				sentry_value_new_bool(i == DisplayServer::get_singleton()->get_primary_screen()));

		CharString orientation;
		switch (DisplayServer::get_singleton()->screen_get_orientation(i)) {
			case DisplayServer::SCREEN_LANDSCAPE: {
				orientation = "Landscape";
			} break;
			case DisplayServer::SCREEN_PORTRAIT: {
				orientation = "Portrait";
			} break;
			case DisplayServer::SCREEN_REVERSE_LANDSCAPE: {
				orientation = "Landscape (reverse)";
			} break;
			case DisplayServer::SCREEN_REVERSE_PORTRAIT: {
				orientation = "Portrait (reverse)";
			} break;
			case DisplayServer::SCREEN_SENSOR_LANDSCAPE: {
				orientation = "Landscape (defined by sensor)";
			} break;
			case DisplayServer::SCREEN_SENSOR_PORTRAIT: {
				orientation = "Portrait (defined by sensor)";
			} break;
			case DisplayServer::SCREEN_SENSOR: {
				orientation = "Defined by sensor";
			} break;
			default: {
				orientation = "Undefined";
			} break;
		}
		sentry_value_set_by_key(screen_data, "orientation", sentry_value_new_string(orientation));

		sentry_value_append(screen_list, screen_data);
	}

	sentry_value_set_by_key(display_context, "screens", screen_list);
	sentry_set_context("display", display_context);
}

CharString Sentry::get_environment() const {
	ERR_FAIL_NULL_V(Engine::get_singleton(), "production");
	if (Engine::get_singleton()->is_editor_hint()) {
		return "editor";
	} else if (OS::get_singleton()->has_feature("editor")) {
		return "editor-run";
	} else if (OS::get_singleton()->is_debug_build()) {
		return "export-debug";
	} else {
		return "export-release";
	}
}

godot::CharString Sentry::get_level_cstring(Level p_level) {
	switch (p_level) {
		case LEVEL_DEBUG: {
			return "debug";
		} break;
		case LEVEL_INFO: {
			return "info";
		} break;
		case LEVEL_WARNING: {
			return "warning";
		} break;
		case LEVEL_ERROR: {
			return "error";
		} break;
		case LEVEL_FATAL: {
			return "fatal";
		} break;
	}
	return "invalid";
}

void Sentry::capture_message(const String &p_message, Level p_level, const String &p_logger) {
	sentry_value_t event = sentry_value_new_message_event(
			(sentry_level_t)p_level,
			p_logger.utf8().get_data(),
			p_message.utf8().get_data());
	last_uuid = sentry_capture_event(event);
}

void Sentry::add_breadcrumb(const godot::String &p_message, const godot::String &p_category, Level p_level,
		const godot::String &p_type, const godot::Dictionary &p_data) {
	sentry_value_t crumb = sentry_value_new_breadcrumb(p_type.utf8().ptr(), p_message.utf8().ptr());
	sentry_value_set_by_key(crumb, "category", sentry_value_new_string(p_category.utf8().ptr()));
	sentry_value_set_by_key(crumb, "level", sentry_value_new_string(get_level_cstring(p_level)));
	sentry_value_set_by_key(crumb, "data", SentryUtil::variant_to_sentry_value(p_data));
	sentry_add_breadcrumb(crumb);
}

String Sentry::get_last_event_id() const {
	char str[37];
	sentry_uuid_as_string(&last_uuid, str);
	return str;
}

void Sentry::set_tag(const godot::String &p_key, const godot::String &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set tag with an empty key.");
	sentry_set_tag(p_key.utf8(), p_value.utf8());
}

void Sentry::remove_tag(const godot::String &p_key) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't remove tag with an empty key.");
	sentry_remove_tag(p_key.utf8());
}

void Sentry::_bind_methods() {
	BIND_ENUM_CONSTANT(LEVEL_DEBUG);
	BIND_ENUM_CONSTANT(LEVEL_INFO);
	BIND_ENUM_CONSTANT(LEVEL_WARNING);
	BIND_ENUM_CONSTANT(LEVEL_ERROR);
	BIND_ENUM_CONSTANT(LEVEL_FATAL);

	ClassDB::bind_method(D_METHOD("capture_message", "message", "level", "logger"), &Sentry::capture_message, DEFVAL(LEVEL_INFO), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("add_breadcrumb", "message", "category", "level", "type", "data"), &Sentry::add_breadcrumb, DEFVAL(LEVEL_INFO), DEFVAL("default"), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_last_event_id"), &Sentry::get_last_event_id);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &Sentry::set_tag);
	ClassDB::bind_method(D_METHOD("remove_tag", "key"), &Sentry::remove_tag);
}

Sentry::Sentry() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());
	ERR_FAIL_NULL(SentrySettings::get_singleton());

	singleton = this;

	if (!SentrySettings::get_singleton()->is_sentry_enabled()) {
		return;
	}

	sentry_options_t *options = sentry_options_new();
	sentry_options_set_dsn(options, SentrySettings::get_singleton()->get_dsn());

	// Establish handler path.
	String handler_fn;
#ifdef LINUX_ENABLED
	handler_fn = "crashpad_handler";
#elif WINDOWS_ENABLED
	handler_fn = "crashpad_handler.exe";
#endif
	String handler_path = OS::get_singleton()->get_executable_path() + "/" + handler_fn;
	if (!FileAccess::file_exists(handler_path)) {
		handler_path = ProjectSettings::get_singleton()->globalize_path("res://addons/sentrysdk/bin/" + handler_fn);
	}
	if (FileAccess::file_exists(handler_path)) {
		sentry_options_set_handler_path(options, handler_path.utf8());
	} else {
		ERR_PRINT("Sentry: Failed to locate crash handler (crashpad)");
		sentry_options_set_backend(options, NULL);
	}

	sentry_options_set_database_path(options, (OS::get_singleton()->get_user_data_dir() + "/sentry").utf8());
	sentry_options_set_sample_rate(options, SentrySettings::get_singleton()->get_sample_rate());
	sentry_options_set_release(options, SentrySettings::get_singleton()->get_release());
	sentry_options_set_debug(options, SentrySettings::get_singleton()->is_debug_printing_enabled());
	sentry_options_set_environment(options, get_environment());
	sentry_options_set_sdk_name(options, "sentry.native.godot");
	sentry_options_set_max_breadcrumbs(options, SentrySettings::get_singleton()->get_max_breadcrumbs());

	// Attach LOG file.
	// TODO: Decide whether log-file must be trimmed before send.
	// TODO: Check if the log file location can be overriden in the project settings.
	if (SentrySettings::get_singleton()->is_attach_log_enabled()) {
		String log_path = OS::get_singleton()->get_user_data_dir() + "/logs/godot.log";
		// if (FileAccess::file_exists(log_path)) {
		sentry_options_add_attachment(options, log_path.utf8());
		// }
	}

	sentry_init(options);
}

Sentry::~Sentry() {
	singleton = nullptr;
	sentry_close();
}
