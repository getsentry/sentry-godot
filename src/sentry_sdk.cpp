#include "sentry_sdk.h"

#include "sentry/environment.h"
#include "sentry/native/native_sdk.h"
#include "sentry_options.h"
#include "sentry_util.h"

// #include <sentry.h>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;
using namespace sentry;

SentrySDK *SentrySDK::singleton = nullptr;

VARIANT_ENUM_CAST(Level);

// TODO: move contexts
void SentrySDK::add_device_context() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(Engine::get_singleton());
	ERR_FAIL_NULL(DisplayServer::get_singleton());
	ERR_FAIL_NULL(Time::get_singleton());

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
	String device_id = runtime_config.get_device_id();
	if (device_id.length() == 0) {
		device_id = SentryUtil::generate_uuid();
		runtime_config.set_device_id(device_id);
	}
	device_context["device_unique_identifier"] = device_id;

	internal_sdk->set_context("device", device_context);
}

void SentrySDK::add_app_context() {
	ERR_FAIL_NULL(Time::get_singleton());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());
	ERR_FAIL_NULL(Engine::get_singleton());

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

	internal_sdk->set_context("app", app_context);
}

void SentrySDK::add_gpu_context() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	ERR_FAIL_NULL(OS::get_singleton());

	Dictionary gpu_context = Dictionary();

	// Note: In headless/server mode, some of these functions return empty strings.
	String adapter_name = RenderingServer::get_singleton()->get_video_adapter_name();
	if (!adapter_name.is_empty()) {
		gpu_context["name"] = RenderingServer::get_singleton()->get_video_adapter_name();
	} else {
		// Since `name` is required for GPU context, don't add the context.
		// We're in the headless environment, most likely.
		return;
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

	internal_sdk->set_context("gpu", gpu_context);
}

void SentrySDK::add_culture_context() {
	ERR_FAIL_NULL(OS::get_singleton());

	Dictionary culture_context = Dictionary();

	culture_context["type"] = "culture";
	culture_context["locale"] = OS::get_singleton()->get_locale();

	String timezone = Time::get_singleton()->get_time_zone_from_system().get("name", "");
	if (!timezone.is_empty()) {
		culture_context["timezone"] = timezone;
	}

	internal_sdk->set_context("culture", culture_context);
}

void SentrySDK::add_display_context() {
	ERR_FAIL_NULL(DisplayServer::get_singleton());

	Dictionary display_context = Dictionary();

	int32_t num_screens = DisplayServer::get_singleton()->get_screen_count();
	display_context["screen_count"] = num_screens;
	display_context["display_server"] = DisplayServer::get_singleton()->get_name();
	display_context["touchscreen_available"] = DisplayServer::get_singleton()->is_touchscreen_available();

	Array screen_list = Array();
	for (int32_t i = 0; i < num_screens; i++) {
		Dictionary screen_data = Dictionary();
		screen_data["size"] = DisplayServer::get_singleton()->screen_get_size(i);
		screen_data["dpi"] = DisplayServer::get_singleton()->screen_get_dpi(i);
		screen_data["refresh_rate"] = DisplayServer::get_singleton()->screen_get_refresh_rate(i);
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
	internal_sdk->set_context("Display", display_context);
}

void SentrySDK::add_engine_context() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(Engine::get_singleton());

	Dictionary godot_context = Dictionary();

	Dictionary version_info = Engine::get_singleton()->get_version_info();
	godot_context["version"] = version_info.get("string", "");
	godot_context["version_commit"] = version_info.get("hash", "");
	godot_context["debug_build"] = OS::get_singleton()->is_debug_build();
	godot_context["command_line_arguments"] = OS::get_singleton()->get_cmdline_args();
	godot_context["mode"] = String(sentry::environment::get_environment());
	godot_context["editor_build"] = OS::get_singleton()->has_feature("editor");

	internal_sdk->set_context("Godot Engine", godot_context);
}

void SentrySDK::add_environment_context() {
	ERR_FAIL_NULL(OS::get_singleton());

	Dictionary env_context = Dictionary();
	env_context["sandboxed"] = OS::get_singleton()->is_sandboxed();
	env_context["userfs_persistent"] = OS::get_singleton()->is_userfs_persistent();
	env_context["granted_permissions"] = OS::get_singleton()->get_granted_permissions();

#ifdef LINUX_ENABLED
	String distribution_name = OS::get_singleton()->get_distribution_name();
	if (!distribution_name.is_empty()) {
		env_context["distribution_name"] = distribution_name;
	}
#endif

	internal_sdk->set_context("Environment", env_context);
}

// sentry_value_t SentrySDK::_create_performance_context() {
// 	sentry_value_t perf_context = sentry_value_new_object();

// 	if (!OS::get_singleton()) {
// 		// Too early, bailing out.
// 		return perf_context;
// 	}

// 	// * Injecting "Performance" context...

// 	// Static memory allocation.
// 	sentry_value_set_by_key(perf_context, "static_memory_peak_usage",
// 			sentry_value_new_string(String::humanize_size(OS::get_singleton()->get_static_memory_peak_usage()).utf8()));
// 	sentry_value_set_by_key(perf_context, "static_memory_usage",
// 			sentry_value_new_string(String::humanize_size(OS::get_singleton()->get_static_memory_usage()).utf8()));

// 	Dictionary meminfo = OS::get_singleton()->get_memory_info();
// 	sentry_value_set_by_key(perf_context, "thread_stack_size",
// 			sentry_value_new_string(String::humanize_size(meminfo["stack"]).utf8()));

// 	if (Engine::get_singleton()) {
// 		// double fps_metric = Engine::get_singleton()->get_frames_per_second();
// 		// if (fps_metric) {
// 		// 	sentry_value_set_by_key(perf_context, "fps",
// 		// 			sentry_value_new_double(fps_metric));
// 		// }

// 		// Frames drawn since engine started - age metric.
// 		// sentry_value_set_by_key(perf_context, "frames_drawn",
// 		// 		sentry_value_new_int32(Engine::get_singleton()->get_frames_drawn()));
// 	}

// 	if (Performance::get_singleton()) {
// 		// Object allocation info.
// 		sentry_value_set_by_key(perf_context, "object_count",
// 				sentry_value_new_int32(Performance::get_singleton()->get_monitor(Performance::OBJECT_COUNT)));
// 		sentry_value_set_by_key(perf_context, "object_node_count",
// 				sentry_value_new_int32(Performance::get_singleton()->get_monitor(Performance::OBJECT_NODE_COUNT)));
// 		sentry_value_set_by_key(perf_context, "object_orphan_node_count",
// 				sentry_value_new_int32(Performance::get_singleton()->get_monitor(Performance::OBJECT_ORPHAN_NODE_COUNT)));
// 		sentry_value_set_by_key(perf_context, "object_resource_count",
// 				sentry_value_new_int32(Performance::get_singleton()->get_monitor(Performance::OBJECT_RESOURCE_COUNT)));
// 	}

// 	if (RenderingServer::get_singleton()) {
// 		// VRAM usage.
// 		uint64_t video_mem_used = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_VIDEO_MEM_USED);
// 		sentry_value_set_by_key(perf_context, "rendering_video_mem_used",
// 				sentry_value_new_string(String::humanize_size(video_mem_used).utf8()));
// 		uint64_t texture_mem = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_TEXTURE_MEM_USED);
// 		sentry_value_set_by_key(perf_context, "rendering_texture_mem_used",
// 				sentry_value_new_string(String::humanize_size(texture_mem).utf8()));
// 		uint64_t buffer_mem = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_BUFFER_MEM_USED);
// 		sentry_value_set_by_key(perf_context, "rendering_buffer_mem_used",
// 				sentry_value_new_string(String::humanize_size(buffer_mem).utf8()));

// 		// Frame statistics.
// 		uint64_t draw_calls = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_TOTAL_DRAW_CALLS_IN_FRAME);
// 		sentry_value_set_by_key(perf_context, "rendering_draw_calls", sentry_value_new_int32(draw_calls));
// 	}

// 	// TODO: Collect more useful metrics: physics, navigation, audio...
// 	// TODO: Q: Split into categories and make it optional?
// 	// TODO: Q: Make performance context optional?
// 	// TODO: Q: Rename it?

// 	return perf_context;
// }

CharString SentrySDK::get_environment() const {
	return sentry::environment::get_environment();
}

void SentrySDK::capture_message(const String &p_message, Level p_level, const String &p_logger) {
	internal_sdk->capture_message(p_message, p_level, p_logger);
}

void SentrySDK::add_breadcrumb(const String &p_message, const String &p_category, Level p_level,
		const String &p_type, const Dictionary &p_data) {
	internal_sdk->add_breadcrumb(p_message, p_category, p_level, p_type, p_data);
}

String SentrySDK::get_last_event_id() const {
	return internal_sdk->get_last_event_id();
}

void SentrySDK::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set tag with an empty key.");
	internal_sdk->set_tag(p_key, p_value);
}

void SentrySDK::remove_tag(const String &p_key) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't remove tag with an empty key.");
	internal_sdk->remove_tag(p_key);
}

void SentrySDK::set_user(const Ref<SentryUser> &p_user) {
	ERR_FAIL_NULL_MSG(p_user, "Sentry: Setting user failed - user object is null. Please, use Sentry.remove_user() to clear user info.");

	// Initialize user ID if not supplied.
	if (p_user->get_id().is_empty()) {
		// Take user ID from the runtime config or generate a new one if it's empty.
		String user_id = get_user()->get_id();
		if (user_id.is_empty()) {
			user_id = SentryUtil::generate_uuid();
		}
		p_user->set_id(user_id);
	}

	// Save user in a runtime conf-file.
	// TODO: Make it optional?
	runtime_config.set_user(p_user);

	internal_sdk->set_user(p_user);
}

void SentrySDK::remove_user() {
	internal_sdk->remove_user();
}

void SentrySDK::set_context(const godot::String &p_key, const godot::Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set context with an empty key.");
	internal_sdk->set_context(p_key, p_value);
}

// TODO: Fix handlers!
// sentry_value_t SentrySDK::handle_before_send(sentry_value_t p_event) {
// 	SentryUtil::sentry_event_set_context(p_event, "Performance", _create_performance_context());
// 	return p_event;
// }

// sentry_value_t SentrySDK::handle_on_crash(sentry_value_t p_event) {
// 	SentryUtil::sentry_event_set_context(p_event, "Performance", _create_performance_context());
// 	return p_event;
// }

void SentrySDK::_bind_methods() {
	BIND_ENUM_CONSTANT(LEVEL_DEBUG);
	BIND_ENUM_CONSTANT(LEVEL_INFO);
	BIND_ENUM_CONSTANT(LEVEL_WARNING);
	BIND_ENUM_CONSTANT(LEVEL_ERROR);
	BIND_ENUM_CONSTANT(LEVEL_FATAL);

	ClassDB::bind_method(D_METHOD("capture_message", "message", "level", "logger"), &SentrySDK::capture_message, DEFVAL(LEVEL_INFO), DEFVAL(""));
	ClassDB::bind_method(D_METHOD("add_breadcrumb", "message", "category", "level", "type", "data"), &SentrySDK::add_breadcrumb, DEFVAL(LEVEL_INFO), DEFVAL("default"), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("get_last_event_id"), &SentrySDK::get_last_event_id);
	ClassDB::bind_method(D_METHOD("set_context", "key", "value"), &SentrySDK::set_context);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &SentrySDK::set_tag);
	ClassDB::bind_method(D_METHOD("remove_tag", "key"), &SentrySDK::remove_tag);
	ClassDB::bind_method(D_METHOD("set_user", "user"), &SentrySDK::set_user);
	ClassDB::bind_method(D_METHOD("get_user"), &SentrySDK::get_user);
	ClassDB::bind_method(D_METHOD("remove_user"), &SentrySDK::remove_user);
}

SentrySDK::SentrySDK() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(ProjectSettings::get_singleton());
	ERR_FAIL_NULL(SentryOptions::get_singleton());

	singleton = this;

#if defined(LINUX_ENABLED) || defined(WINDOWS_ENABLED) || defined(MACOS_ENABLED)
	internal_sdk = std::make_shared<NativeSDK>();
#else
	// Unsupported platform
	// TODO: Create fake SDK?
	return;
#endif
	internal_sdk->initialize();

	// Setup logging.

	if (!SentryOptions::get_singleton()->is_enabled()) {
		return;
	}

	// Load the runtime configuration from the user's data directory.
	runtime_config.load_file(OS::get_singleton()->get_user_data_dir() + "/sentry.dat");

	// TODO: Fix hooks!
	// "before_send" hook.
	// auto before_send_lambda = [](sentry_value_t event, void *hint, void *closure) {
	// 	return SentrySDK::get_singleton()->handle_before_send(event);
	// };
	// sentry_options_set_before_send(options, before_send_lambda, NULL);

	// "on_crash" hook.
	// auto on_crash_lambda = [](const sentry_ucontext_t *uctx, sentry_value_t event, void *closure) {
	// 	return SentrySDK::get_singleton()->handle_on_crash(event);
	// };
	// sentry_options_set_on_crash(options, on_crash_lambda, NULL);

	// Initialize user.
	set_user(runtime_config.get_user());
}

SentrySDK::~SentrySDK() {
	singleton = nullptr;
}
