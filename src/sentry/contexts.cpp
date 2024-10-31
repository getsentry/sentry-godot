#include "contexts.h"

#include "../sentry_util.h"
#include "environment.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace sentry::contexts {

Dictionary make_device_context(const Ref<RuntimeConfig> &p_runtime_config) {
	ERR_FAIL_NULL_V(OS::get_singleton(), Dictionary());
	ERR_FAIL_NULL_V(Engine::get_singleton(), Dictionary());
	ERR_FAIL_NULL_V(DisplayServer::get_singleton(), Dictionary());
	ERR_FAIL_NULL_V(Time::get_singleton(), Dictionary());

	Dictionary device_context = Dictionary();
	device_context["arch"] = Engine::get_singleton()->get_architecture_name();
	int primary_screen = DisplayServer::get_singleton()->get_primary_screen();
	String orientation = SentryUtil::get_screen_orientation_string(primary_screen);
	if (orientation.length() > 0) {
		device_context["orientation"] = orientation;
	}

	// TODO: Need platform-specific solutions - this doesn't work well.
	String host = OS::get_singleton()->get_environment("HOST");
	if (host.is_empty()) {
		host = "localhost";
	}
	device_context["name"] = host;

	String model = OS::get_singleton()->get_model_name();
	if (!model.is_empty() && model != "GenericDevice") {
		device_context["model"] = model;
	}

	Vector2i resolution = DisplayServer::get_singleton()->screen_get_size(primary_screen);
	device_context["screen_width_pixels"] = resolution.x;
	device_context["screen_height_pixels"] = resolution.y;
	device_context["screen_dpi"] = DisplayServer::get_singleton()->screen_get_dpi(
			DisplayServer::get_singleton()->get_primary_screen());

	Dictionary meminfo = OS::get_singleton()->get_memory_info();
	// Note: Using double since int32 can't handle size in bytes.
	device_context["memory_size"] = double(meminfo["physical"]);
	device_context["free_memory"] = double(meminfo["free"]);
	device_context["usable_memory"] = double(meminfo["available"]);

	auto dir = DirAccess::open("user://");
	if (dir.is_valid()) {
		device_context["free_storage"] = double(dir->get_space_left());
	}

	// TODO: device type.

	device_context["processor_count"] = OS::get_singleton()->get_processor_count();
	device_context["cpu_description"] = OS::get_singleton()->get_processor_name();

	// Read/initialize device unique identifier.
	String device_id = p_runtime_config->get_device_id();
	if (device_id.length() == 0) {
		device_id = SentryUtil::generate_uuid();
		p_runtime_config->set_device_id(device_id);
	}
	device_context["device_unique_identifier"] = device_id;

	return device_context;
}

Dictionary make_app_context() {
	ERR_FAIL_NULL_V(Time::get_singleton(), Dictionary());
	ERR_FAIL_NULL_V(ProjectSettings::get_singleton(), Dictionary());
	ERR_FAIL_NULL_V(Engine::get_singleton(), Dictionary());

	Dictionary app_context = Dictionary();

	double time = Time::get_singleton()->get_unix_time_from_system();
	time -= Time::get_singleton()->get_ticks_msec() * 0.001;
	String start_time = Time::get_singleton()->get_datetime_string_from_unix_time(time, true);
	app_context["app_start_time"] = start_time;

	String app_name = ProjectSettings::get_singleton()->get_setting("application/config/name", "Unnamed project");
	app_context["app_name"] = app_name;

	String app_version = ProjectSettings::get_singleton()->get_setting("application/config/version", "");
	if (!app_version.is_empty()) {
		app_context["app_version"] = app_version;
	}

	app_context["app_arch"] = Engine::get_singleton()->get_architecture_name();

	return app_context;
}

Dictionary make_gpu_context() {
	ERR_FAIL_NULL_V(RenderingServer::get_singleton(), Dictionary());
	ERR_FAIL_NULL_V(OS::get_singleton(), Dictionary());

	Dictionary gpu_context = Dictionary();

	// Note: In headless/server mode, some of these functions return empty strings.
	String adapter_name = RenderingServer::get_singleton()->get_video_adapter_name();
	if (!adapter_name.is_empty()) {
		gpu_context["name"] = RenderingServer::get_singleton()->get_video_adapter_name();
	} else {
		// Since `name` is required for GPU context, don't add the context.
		// We're in the headless environment, most likely.
		return Dictionary();
	}

	String adapter_vendor = RenderingServer::get_singleton()->get_video_adapter_vendor();
	if (!adapter_vendor.is_empty()) {
		gpu_context["vendor_name"] = adapter_vendor;
	}

	String api_version = RenderingServer::get_singleton()->get_video_adapter_api_version();
	if (!api_version.is_empty()) {
		gpu_context["version"] = api_version;
	}

	// Device type.
	// TODO: Custom key. Keep or remove?
	String device_type = "Unknown";
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
	gpu_context["device_type"] = device_type;

	// Driver info.
	PackedStringArray driver_info = OS::get_singleton()->get_video_adapter_driver_info();
	if (driver_info.size() >= 2) {
		gpu_context["driver_name"] = driver_info[0];
		gpu_context["driver_version"] = driver_info[1];
	}

	return gpu_context;
}

Dictionary make_culture_context() {
	Dictionary culture_context = Dictionary();
	ERR_FAIL_NULL_V(OS::get_singleton(), culture_context);

	culture_context["type"] = "culture";
	culture_context["locale"] = OS::get_singleton()->get_locale();

	String timezone = Time::get_singleton()->get_time_zone_from_system().get("name", "");
	if (!timezone.is_empty()) {
		culture_context["timezone"] = timezone;
	}

	return culture_context;
}

Dictionary make_display_context() {
	Dictionary display_context = Dictionary();
	ERR_FAIL_NULL_V(DisplayServer::get_singleton(), display_context);

	int32_t num_screens = DisplayServer::get_singleton()->get_screen_count();
	display_context["screen_count"] = num_screens;
	display_context["display_server"] = DisplayServer::get_singleton()->get_name();
	display_context["touchscreen_available"] = DisplayServer::get_singleton()->is_touchscreen_available();

	Array screen_list = Array();
	for (int32_t i = 0; i < num_screens; i++) {
		Dictionary screen_data = Dictionary();
		screen_data["size"] = DisplayServer::get_singleton()->screen_get_size(i);
		screen_data["dpi"] = DisplayServer::get_singleton()->screen_get_dpi(i);
		screen_data["refresh_rate"] = Math::snapped(DisplayServer::get_singleton()->screen_get_refresh_rate(i), 0.01);
		screen_data["position"] = DisplayServer::get_singleton()->screen_get_position(i);
		screen_data["scale_factor"] = DisplayServer::get_singleton()->screen_get_scale(i);
		screen_data["primary"] = i == DisplayServer::get_singleton()->get_primary_screen();

		String orientation = SentryUtil::get_screen_orientation_string(i);
		if (!orientation.is_empty()) {
			screen_data["orientation"] = orientation;
		}

		screen_list.append(screen_data);
	}

	display_context["screens"] = screen_list;
	return display_context;
}

Dictionary make_godot_engine_context() {
	Dictionary godot_context = Dictionary();
	ERR_FAIL_NULL_V(OS::get_singleton(), godot_context);
	ERR_FAIL_NULL_V(Engine::get_singleton(), godot_context);

	Dictionary version_info = Engine::get_singleton()->get_version_info();
	godot_context["version"] = version_info.get("string", "");
	godot_context["version_commit"] = version_info.get("hash", "");
	godot_context["debug_build"] = OS::get_singleton()->is_debug_build();
	godot_context["command_line_arguments"] = OS::get_singleton()->get_cmdline_args();
	godot_context["mode"] = String(sentry::environment::get_environment());
	godot_context["editor_build"] = OS::get_singleton()->has_feature("editor");

	return godot_context;
}

Dictionary make_environment_context() {
	Dictionary env_context = Dictionary();
	ERR_FAIL_NULL_V(OS::get_singleton(), env_context);

	env_context["sandboxed"] = OS::get_singleton()->is_sandboxed();
	env_context["userfs_persistent"] = OS::get_singleton()->is_userfs_persistent();
	env_context["granted_permissions"] = OS::get_singleton()->get_granted_permissions();

#ifdef LINUX_ENABLED
	String distribution_name = OS::get_singleton()->get_distribution_name();
	if (!distribution_name.is_empty()) {
		env_context["distribution_name"] = distribution_name;
	}
#endif

	return env_context;
}

} //namespace sentry::contexts
