#include "contexts.h"

#include "gen/sdk_version.gen.h"
#include "sentry/environment.h"
#include "sentry/godot_singletons.h"
#include "sentry/sentry_sdk.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/time.hpp>

#ifdef LINUX_ENABLED
#include <limits.h>
#include <unistd.h>
#endif // LINUX_ENABLED

#ifdef WINDOWS_ENABLED
#include <windows.h>
#endif // WINDOWS_ENABLED

namespace {

String _screen_orientation_as_string(int32_t p_screen) {
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

String _get_hostname() {
#ifdef LINUX_ENABLED
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif
	char buffer[HOST_NAME_MAX + 1];
	if (gethostname(buffer, sizeof(buffer)) == 0) {
		buffer[sizeof(buffer) - 1] = '\0'; // ensure termination
		return String::utf8(buffer);
	}
#elif WINDOWS_ENABLED
	wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = sizeof(buffer) / sizeof(buffer[0]);

	if (GetComputerNameW(buffer, &size)) {
		return String::utf16((const char16_t *)(buffer), size);
	}
#endif
	return String();
}

} // unnamed namespace

namespace sentry::contexts {

Dictionary make_device_context(const Ref<RuntimeConfig> &p_runtime_config) {
	Dictionary device_context = Dictionary();
	ERR_FAIL_NULL_V(OS::get_singleton(), device_context);
	ERR_FAIL_NULL_V(Engine::get_singleton(), device_context);
	ERR_FAIL_NULL_V(DisplayServer::get_singleton(), device_context);

	device_context["arch"] = Engine::get_singleton()->get_architecture_name();
	int primary_screen = DisplayServer::get_singleton()->get_primary_screen();
	String orientation = _screen_orientation_as_string(primary_screen);
	if (orientation.length() > 0) {
		device_context["orientation"] = orientation;
	}

	// Set device.name to hostname if server.
	if (SENTRY_OPTIONS()->is_send_default_pii_enabled() && OS::get_singleton()->has_feature("dedicated_server")) {
		String host = _get_hostname();
		if (!host.is_empty()) {
			device_context["name"] = host;
		}
	}

	String model = OS::get_singleton()->get_model_name();
	if (!model.is_empty() && model != "GenericDevice") {
		device_context["model"] = model;
	}

	// Note: Godot Engine doesn't support changing screen resolution.
	Vector2i resolution = DisplayServer::get_singleton()->screen_get_size(primary_screen);
	device_context["screen_width_pixels"] = resolution.x;
	device_context["screen_height_pixels"] = resolution.y;
	device_context["screen_dpi"] = DisplayServer::get_singleton()->screen_get_dpi(
			DisplayServer::get_singleton()->get_primary_screen());

#if !defined(IOS_ENABLED)
	// NOTE: Memory info access on iOS can cause runtime errors in Godot 4.5.
	// See: https://github.com/godotengine/godot/issues/109073
	Dictionary meminfo = OS::get_singleton()->get_memory_info();
	int64_t mem_size = meminfo["physical"];
	int64_t mem_free = meminfo["free"];
	int64_t mem_usable = meminfo["available"];
	if (mem_size > 0) {
		device_context["memory_size"] = mem_size;
	}
	if (mem_free >= 0) {
		device_context["free_memory"] = mem_free;
	}
	if (mem_usable >= 0) {
		device_context["usable_memory"] = mem_usable;
	}
#endif // !IOS_ENABLED

	auto dir = DirAccess::open("user://");
	if (dir.is_valid()) {
		device_context["free_storage"] = dir->get_space_left();
	}

	device_context["processor_count"] = OS::get_singleton()->get_processor_count();

	String cpu_desc = OS::get_singleton()->get_processor_name();
	if (!cpu_desc.is_empty()) {
		device_context["cpu_description"] = cpu_desc;
	}

	// Read/initialize installation id.
	String installation_id = p_runtime_config->get_installation_id();
	device_context["device_unique_identifier"] = installation_id;

	return device_context;
}

Dictionary make_device_context_update() {
	Dictionary device_context = Dictionary();
	ERR_FAIL_NULL_V(DisplayServer::get_singleton(), device_context);
	ERR_FAIL_NULL_V(OS::get_singleton(), device_context);

	int primary_screen = DisplayServer::get_singleton()->get_primary_screen();
	device_context["orientation"] = _screen_orientation_as_string(primary_screen);

#if !defined(IOS_ENABLED)
	// NOTE: Memory info access on iOS can cause runtime errors in Godot 4.5.
	const Dictionary &meminfo = OS::get_singleton()->get_memory_info();
	int64_t mem_free = meminfo["free"];
	int64_t mem_usable = meminfo["available"];
	if (mem_free >= 0) {
		device_context["free_memory"] = mem_free;
	}
	if (mem_usable >= 0) {
		device_context["usable_memory"] = mem_usable;
	}
#endif // !IOS_ENABLED

	auto dir = DirAccess::open("user://");
	if (dir.is_valid()) {
		device_context["free_storage"] = dir->get_space_left();
	}

	return device_context;
}

Dictionary make_app_context() {
	Dictionary app_context = Dictionary();
	ERR_FAIL_NULL_V(Time::get_singleton(), app_context);
	ERR_FAIL_NULL_V(ProjectSettings::get_singleton(), app_context);
	ERR_FAIL_NULL_V(Engine::get_singleton(), app_context);

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
	Dictionary gpu_context = Dictionary();

	ERR_FAIL_NULL_V(OS::get_singleton(), gpu_context);
	ERR_FAIL_NULL_V(RenderingServer::get_singleton(), gpu_context);

	// Note: In headless/server mode, some of these functions return empty strings.
	String adapter_name = RenderingServer::get_singleton()->get_video_adapter_name();
	if (!adapter_name.is_empty()) {
		gpu_context["name"] = adapter_name;
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

		String orientation = _screen_orientation_as_string(i);
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
	godot_context["mode"] = sentry::environment::detect_godot_environment();
	godot_context["editor_build"] = OS::get_singleton()->has_feature("editor");
	godot_context["godot_sdk_version"] = String(SENTRY_GODOT_SDK_VERSION);

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

Dictionary make_performance_context() {
	Dictionary perf_context = Dictionary();
	ERR_FAIL_NULL_V(OS::get_singleton(), perf_context);
	ERR_FAIL_NULL_V(Engine::get_singleton(), perf_context);
	ERR_FAIL_NULL_V(Performance::get_singleton(), perf_context);
	ERR_FAIL_NULL_V(RenderingServer::get_singleton(), perf_context);

	// * Injecting "Performance" context...

	// Static memory allocation.
	perf_context["static_memory_peak_usage"] = String::humanize_size(OS::get_singleton()->get_static_memory_peak_usage());
	perf_context["static_memory_usage"] = String::humanize_size(OS::get_singleton()->get_static_memory_usage());

#ifndef IOS_ENABLED
	// NOTE: Memory info access on iOS can cause runtime errors in Godot 4.5.
	Dictionary meminfo = OS::get_singleton()->get_memory_info();
	perf_context["main_thread_stack_size"] = String::humanize_size(meminfo["stack"]);
#endif // !IOS_ENABLED

	double fps_metric = Engine::get_singleton()->get_frames_per_second();
	if (fps_metric) {
		perf_context["fps"] = fps_metric;
	}

	// Frames drawn since engine started - age metric.
	perf_context["frames_drawn"] = Engine::get_singleton()->get_frames_drawn();

	// Object allocation info.
	perf_context["object_count"] = Performance::get_singleton()->get_monitor(Performance::OBJECT_COUNT);
	perf_context["object_node_count"] = Performance::get_singleton()->get_monitor(Performance::OBJECT_NODE_COUNT);
	perf_context["object_orphan_node_count"] = Performance::get_singleton()->get_monitor(Performance::OBJECT_ORPHAN_NODE_COUNT);
	perf_context["object_resource_count"] = Performance::get_singleton()->get_monitor(Performance::OBJECT_RESOURCE_COUNT);

	// VRAM usage.
	uint64_t video_mem_used = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_VIDEO_MEM_USED);
	perf_context["rendering_video_mem_used"] = String::humanize_size(video_mem_used);
	uint64_t texture_mem = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_TEXTURE_MEM_USED);
	perf_context["rendering_texture_mem_used"] = String::humanize_size(texture_mem);
	uint64_t buffer_mem = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_BUFFER_MEM_USED);
	perf_context["rendering_buffer_mem_used"] = String::humanize_size(buffer_mem);

	// Frame statistics.
	uint64_t draw_calls = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_TOTAL_DRAW_CALLS_IN_FRAME);
	perf_context["rendering_draw_calls"] = draw_calls;

	// TODO: Collect more useful metrics: physics, navigation, audio...
	// TODO: Q: Split into categories and make it optional?
	// TODO: Q: Make performance context optional?
	// TODO: Q: Rename it?

	return perf_context;
}

HashMap<String, Dictionary> make_event_contexts() {
	HashMap<String, Dictionary> event_contexts;

	if (!sentry::godot_singletons::are_ready()) {
		// Engine singletons may not be fully initialized yet - skip context enrichment.
		// This can happen when processing error reports early in the app lifecycle.
		return event_contexts;
	}

	event_contexts["godot_performance"] = make_performance_context();

#if defined(SDK_NATIVE) || defined(SDK_JAVASCRIPT)
	event_contexts["device"] = make_device_context_update();
#endif

	return event_contexts;
}

} //namespace sentry::contexts
