#include "sentry_singleton.h"

#include "sentry.h"
#include "sentry_settings.h"
#include "sentry_util.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

Sentry *Sentry::singleton = nullptr;

void Sentry::add_gpu_context() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	ERR_FAIL_NULL(OS::get_singleton());

	sentry_value_t gpu_context = sentry_value_new_object();

	// Note: In headless/server mode, some of these functions return empty strings.
	sentry_value_set_by_key(gpu_context, "name",
			sentry_value_new_string(RenderingServer::get_singleton()->get_video_adapter_name().utf8()));
	sentry_value_set_by_key(gpu_context, "vendor_name",
			sentry_value_new_string(RenderingServer::get_singleton()->get_video_adapter_vendor().utf8()));
	sentry_value_set_by_key(gpu_context, "version",
			sentry_value_new_string(RenderingServer::get_singleton()->get_video_adapter_api_version().utf8()));

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

	// Driver info.
	PackedStringArray driver_info = OS::get_singleton()->get_video_adapter_driver_info();
	if (driver_info.size() >= 2) {
		sentry_value_set_by_key(gpu_context, "driver_name", sentry_value_new_string(driver_info[0].utf8()));
		sentry_value_set_by_key(gpu_context, "driver_version", sentry_value_new_string(driver_info[1].utf8()));
	}

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

		CharString orientation = SentryUtil::get_screen_orientation_cstring(i);
		if (orientation.size() > 0) {
			sentry_value_set_by_key(screen_data, "orientation", sentry_value_new_string(orientation));
		}

		sentry_value_append(screen_list, screen_data);
	}

	sentry_value_set_by_key(display_context, "screens", screen_list);
	sentry_set_context("display", display_context);
}

void Sentry::add_engine_context() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(Engine::get_singleton());

	sentry_value_t godot_context = sentry_value_new_object();
	sentry_value_set_by_key(godot_context, "version",
			sentry_value_new_string(Engine::get_singleton()->get_version_info()["string"].stringify().utf8()));
	sentry_value_set_by_key(godot_context, "debug_build",
			sentry_value_new_bool(OS::get_singleton()->is_debug_build()));
	sentry_value_set_by_key(godot_context, "executable_path",
			sentry_value_new_string(OS::get_singleton()->get_executable_path().utf8()));
	sentry_value_set_by_key(godot_context, "command_line_arguments",
			SentryUtil::variant_to_sentry_value(OS::get_singleton()->get_cmdline_args()));
	sentry_value_set_by_key(godot_context, "mode", sentry_value_new_string(get_environment()));

	sentry_set_context("Godot Engine", godot_context);
}

void Sentry::add_environment_context() {
	ERR_FAIL_NULL(OS::get_singleton());

	sentry_value_t env_context = sentry_value_new_object();
	sentry_value_set_by_key(env_context, "name",
			sentry_value_new_string(OS::get_singleton()->get_name().utf8()));
	sentry_value_set_by_key(env_context, "sandboxed",
			sentry_value_new_bool(OS::get_singleton()->is_sandboxed()));
	sentry_value_set_by_key(env_context, "user_data_dir",
			sentry_value_new_string(OS::get_singleton()->get_user_data_dir().utf8()));
	sentry_value_set_by_key(env_context, "userfs_persistent",
			sentry_value_new_bool(OS::get_singleton()->is_userfs_persistent()));
	sentry_value_set_by_key(env_context, "granted_permissions",
			SentryUtil::variant_to_sentry_value(OS::get_singleton()->get_granted_permissions()));
	sentry_value_set_by_key(env_context, "locale",
			sentry_value_new_string(OS::get_singleton()->get_locale().utf8()));

	String distribution_name = OS::get_singleton()->get_distribution_name();
	if (!distribution_name.is_empty()) {
		sentry_value_set_by_key(env_context, "distribution_name",
				sentry_value_new_string(distribution_name.utf8()));
	}

	String version = OS::get_singleton()->get_version();
	if (!version.is_empty()) {
		sentry_value_set_by_key(env_context, "version",
				sentry_value_new_string(version.utf8()));
	}

	sentry_set_context("Environment", env_context);
}

void Sentry::add_device_context() {
	ERR_FAIL_NULL(OS::get_singleton());
	ERR_FAIL_NULL(Engine::get_singleton());
	ERR_FAIL_NULL(DisplayServer::get_singleton());

	sentry_value_t device_context = sentry_value_new_object();

	sentry_value_set_by_key(device_context, "arch",
			sentry_value_new_string(Engine::get_singleton()->get_architecture_name().utf8()));

	int primary_screen = DisplayServer::get_singleton()->get_primary_screen();
	CharString orientation = SentryUtil::get_screen_orientation_cstring(primary_screen);
	if (orientation.size() > 0) {
		sentry_value_set_by_key(device_context, "orientation", sentry_value_new_string(orientation));
	}

	String host = OS::get_singleton()->get_environment("HOST");
	if (host.is_empty()) {
		host = "localhost";
	}
	sentry_value_set_by_key(device_context, "name", sentry_value_new_string(host.utf8()));

	String model = OS::get_singleton()->get_model_name();
	if (!model.is_empty()) {
		sentry_value_set_by_key(device_context, "model", sentry_value_new_string(model.utf8()));
	}

	Vector2i resolution = DisplayServer::get_singleton()->screen_get_size(primary_screen);
	sentry_value_set_by_key(device_context, "screen_width_pixels", sentry_value_new_int32(resolution.x));
	sentry_value_set_by_key(device_context, "screen_height_pixels", sentry_value_new_int32(resolution.y));

	// Note: Custom key.
	double refresh_rate = DisplayServer::get_singleton()->screen_get_refresh_rate(primary_screen);
	refresh_rate = Math::snapped(refresh_rate, 0.01);
	sentry_value_set_by_key(device_context, "screen_refresh_rate", sentry_value_new_double(refresh_rate));

	sentry_value_set_by_key(device_context, "screen_dpi",
			sentry_value_new_int32(
					DisplayServer::get_singleton()->screen_get_dpi(
							DisplayServer::get_singleton()->get_primary_screen())));

	Dictionary meminfo = OS::get_singleton()->get_memory_info();
	// Note: Using custom keys because int32 can't handle size in bytes.
	// TODO: int32 overflows...
	sentry_value_set_by_key(device_context, "memory_physical",
			sentry_value_new_string(String::humanize_size(meminfo["physical"]).utf8()));
	sentry_value_set_by_key(device_context, "memory_free",
			sentry_value_new_string(String::humanize_size(meminfo["free"]).utf8()));
	sentry_value_set_by_key(device_context, "memory_stack",
			sentry_value_new_string(String::humanize_size(meminfo["stack"]).utf8()));
	sentry_value_set_by_key(device_context, "memory_available",
			sentry_value_new_string(String::humanize_size(meminfo["available"]).utf8()));

	sentry_value_set_by_key(device_context, "processor_count",
			sentry_value_new_int32(OS::get_singleton()->get_processor_count()));
	sentry_value_set_by_key(device_context, "cpu_description",
			sentry_value_new_string(OS::get_singleton()->get_processor_name().utf8()));

	// Custom keys.
	sentry_value_set_by_key(device_context, "has_touchscreen",
			sentry_value_new_bool(DisplayServer::get_singleton()->is_touchscreen_available()));
	sentry_value_set_by_key(device_context, "has_virtual_keyboard",
			sentry_value_new_bool(DisplayServer::get_singleton()->has_feature(DisplayServer::FEATURE_VIRTUAL_KEYBOARD)));

	auto dir = DirAccess::open("user://");
	if (dir.is_valid()) {
		sentry_value_set_by_key(device_context, "userfs_free_space",
				sentry_value_new_string(String::humanize_size(dir->get_space_left()).utf8()));
	}

	sentry_set_context("device", device_context);
}

sentry_value_t Sentry::_create_performance_context() {
	sentry_value_t perf_context = sentry_value_new_object();

	if (!OS::get_singleton()) {
		// Too early, bailing out.
		return perf_context;
	}

	// * Injecting "Performance" context...

	// Static memory allocation.
	sentry_value_set_by_key(perf_context, "static_memory_peak_usage",
			sentry_value_new_string(String::humanize_size(OS::get_singleton()->get_static_memory_peak_usage()).utf8()));
	sentry_value_set_by_key(perf_context, "static_memory_usage",
			sentry_value_new_string(String::humanize_size(OS::get_singleton()->get_static_memory_usage()).utf8()));

	if (Engine::get_singleton()) {
		double fps_metric = Engine::get_singleton()->get_frames_per_second();
		if (fps_metric) {
			sentry_value_set_by_key(perf_context, "fps",
					sentry_value_new_double(fps_metric));
		}

		// Frames drawn since engine started - age metric.
		sentry_value_set_by_key(perf_context, "frames_drawn",
				sentry_value_new_int32(Engine::get_singleton()->get_frames_drawn()));
	}

	if (Performance::get_singleton()) {
		// Object allocation info.
		sentry_value_set_by_key(perf_context, "object_count",
				sentry_value_new_int32(Performance::get_singleton()->get_monitor(Performance::OBJECT_COUNT)));
		sentry_value_set_by_key(perf_context, "object_node_count",
				sentry_value_new_int32(Performance::get_singleton()->get_monitor(Performance::OBJECT_NODE_COUNT)));
		sentry_value_set_by_key(perf_context, "object_orphan_node_count",
				sentry_value_new_int32(Performance::get_singleton()->get_monitor(Performance::OBJECT_ORPHAN_NODE_COUNT)));
		sentry_value_set_by_key(perf_context, "object_resource_count",
				sentry_value_new_int32(Performance::get_singleton()->get_monitor(Performance::OBJECT_RESOURCE_COUNT)));
	}

	if (RenderingServer::get_singleton()) {
		// VRAM usage.
		uint64_t video_mem_used = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_VIDEO_MEM_USED);
		sentry_value_set_by_key(perf_context, "rendering_video_mem_used",
				sentry_value_new_string(String::humanize_size(video_mem_used).utf8()));
		uint64_t texture_mem = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_TEXTURE_MEM_USED);
		sentry_value_set_by_key(perf_context, "rendering_texture_mem_used",
				sentry_value_new_string(String::humanize_size(texture_mem).utf8()));
		uint64_t buffer_mem = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_BUFFER_MEM_USED);
		sentry_value_set_by_key(perf_context, "rendering_buffer_mem_used",
				sentry_value_new_string(String::humanize_size(buffer_mem).utf8()));

		// Frame statistics.
		uint64_t draw_calls = RenderingServer::get_singleton()->get_rendering_info(RenderingServer::RENDERING_INFO_TOTAL_DRAW_CALLS_IN_FRAME);
		sentry_value_set_by_key(perf_context, "rendering_draw_calls", sentry_value_new_int32(draw_calls));
	}

	// TODO: Collect more useful metrics: physics, navigation, audio...
	// TODO: Split into categories and make it optional?
	// TODO: Make performance context optional?

	return perf_context;
}

CharString Sentry::get_environment() const {
	ERR_FAIL_NULL_V(Engine::get_singleton(), "production");
	ERR_FAIL_NULL_V(OS::get_singleton(), "production");

	if (OS::get_singleton()->has_feature("dedicated_server")) {
		return "dedicated_server";
	} else if (Engine::get_singleton()->is_editor_hint()) {
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

void Sentry::set_context(const godot::String &p_key, const godot::Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set context with an empty key.");
	sentry_set_context(p_key.utf8(), SentryUtil::variant_to_sentry_value(p_value));
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
	ClassDB::bind_method(D_METHOD("set_context", "key", "value"), &Sentry::set_context);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &Sentry::set_tag);
	ClassDB::bind_method(D_METHOD("remove_tag", "key"), &Sentry::remove_tag);
}

sentry_value_t Sentry::_before_send(sentry_value_t p_event) {
	SentryUtil::sentry_event_set_context(p_event, "Performance", _create_performance_context());
	return p_event;
}

sentry_value_t Sentry::_on_crash(sentry_value_t p_event) {
	SentryUtil::sentry_event_set_context(p_event, "Performance", _create_performance_context());
	return p_event;
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
#elif MACOS_ENABLED
	// TODO: macOS handler?
	handler_fn = "crashpad_handler";
#endif
	String handler_path = OS::get_singleton()->get_executable_path() + "/" + handler_fn;
	if (!FileAccess::file_exists(handler_path)) {
		handler_path = ProjectSettings::get_singleton()->globalize_path("res://addons/sentrysdk/bin/" + handler_fn);
	}
	if (FileAccess::file_exists(handler_path)) {
		sentry_options_set_handler_path(options, handler_path.utf8());
	} else {
		ERR_PRINT("Sentry: Failed to locate crash handler (crashpad) - backend disabled.");
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
	if (SentrySettings::get_singleton()->is_attach_log_enabled()) {
		String log_path = ProjectSettings::get_singleton()->get_setting("debug/file_logging/log_path");
		if (FileAccess::file_exists(log_path)) {
			log_path = log_path.replace("user://", OS::get_singleton()->get_user_data_dir() + "/");
			sentry_options_add_attachment(options, log_path.utf8());
		} else {
			WARN_PRINT("Sentry: Log file not found. Make sure \"debug/file_logging/enable_file_logging\" is turned ON in the Project Settings.");
		}
	}

	// "before_send" hook.
	auto before_send_lambda = [](sentry_value_t event, void *hint, void *closure) {
		return Sentry::get_singleton()->_before_send(event);
	};
	sentry_options_set_before_send(options, before_send_lambda, NULL);

	// "on_crash" hook.
	auto on_crash_lambda = [](const sentry_ucontext_t *uctx, sentry_value_t event, void *closure) {
		return Sentry::get_singleton()->_on_crash(event);
	};
	sentry_options_set_on_crash(options, on_crash_lambda, NULL);

	sentry_init(options);
}

Sentry::~Sentry() {
	singleton = nullptr;
	sentry_close();
}
