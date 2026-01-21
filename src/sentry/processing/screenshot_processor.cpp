#include "screenshot_processor.h"

#include "sentry/common_defs.h"
#include "sentry/godot_singletons.h"
#include "sentry/logging/print.h"
#include "sentry/sentry_options.h"
#include "sentry/util/screenshot.h" // TODO: incorporate

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>

namespace sentry {

Ref<SentryEvent> ScreenshotProcessor::process_event(const Ref<SentryEvent> &p_event) {
	if (p_event.is_null()) {
		sentry::logging::print_error("internal error: can't process null event");
		return nullptr;
	}

	int32_t current_frame = Engine::get_singleton()->get_frames_drawn();

	{
		std::lock_guard lock{ mutex };

		if (current_frame == last_screenshot_frame) {
			sentry::logging::print_debug("Skipping screenshot - already processed this frame");
			return p_event;
		}

		// Remove the outdated screenshot.
		DirAccess::remove_absolute(screenshot_path);
	}

	if (OS::get_singleton()->get_thread_caller_id() != OS::get_singleton()->get_main_thread_id()) {
		sentry::logging::print_debug("Skipping screenshot - can only be performed on the main thread");
		return p_event;
	}

	if (!sentry::godot_singletons::are_ready()) {
		sentry::logging::print_debug("Skipping screenshot - too early in the app lifecycle");
		return p_event;
	}

#if defined(SDK_COCOA) || defined(SDK_ANDROID)
	if (p_event->is_crash()) {
		sentry::logging::print_debug("Skipping screenshot - crash from previous session");
		return p_event;
	}
#endif

	if (DisplayServer::get_singleton()->get_name() == "headless") {
		sentry::logging::print_debug("Skipping screenshot - headless mode");
		return p_event;
	}

	if (p_event->get_level() < SentryOptions::get_singleton()->get_screenshot_level()) {
		sentry::logging::print_debug("Skipping screenshot - screenshot level not met");
		return p_event;
	}

	if (SentryOptions::get_singleton()->get_before_capture_screenshot().is_valid()) {
		Variant result = SentryOptions::get_singleton()->get_before_capture_screenshot().call(p_event);
		if (result.get_type() != Variant::BOOL) {
			// Note: Using PRINT_ONCE to avoid feedback loop in case of error event.
			ERR_PRINT_ONCE("Sentry: before_capture_screenshot callback failed: expected a boolean return value");
			return p_event;
		}
		if (result.operator bool() == false) {
			sentry::logging::print_debug("Skipping screenshot - before_capture_screenshot returned false");
			return p_event;
		}
	}

	mutex.lock();
	last_screenshot_frame = current_frame;
	mutex.unlock();

	sentry::logging::print_debug("Taking screenshot");
	PackedByteArray buffer = sentry::util::take_screenshot();

	Ref<FileAccess> f = FileAccess::open(screenshot_path, FileAccess::WRITE);
	if (f.is_valid()) {
		f->store_buffer(buffer);
		f->flush();
		f->close();
	} else {
		sentry::logging::print_error("Failed to save ", screenshot_path);
	}

	return p_event;
}

ScreenshotProcessor::ScreenshotProcessor() {
	screenshot_path = "user://" SENTRY_SCREENSHOT_FN;
}

} // namespace sentry
