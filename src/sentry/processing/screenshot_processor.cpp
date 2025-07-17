#include "screenshot_processor.h"

#include "sentry/common_defs.h"
#include "sentry/sentry_options.h"
#include "sentry/util/print.h"
#include "sentry/util/screenshot.h" // TODO: incorporate

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>

namespace sentry {

Ref<SentryEvent> ScreenshotProcessor::process_event(const Ref<SentryEvent> &p_event) {
	if (p_event.is_null()) {
		sentry::util::print_error("internal error: can't process null event");
		return nullptr;
	}

	if (!DisplayServer::get_singleton() || DisplayServer::get_singleton()->get_name() == "headless") {
		return p_event;
	}

	int32_t current_frame = Engine::get_singleton()->get_frames_drawn();

	{
		std::lock_guard lock{ mutex };

		if (current_frame == last_screenshot_frame) {
			sentry::util::print_debug("skipping screenshot – already taken");
			return p_event;
		}

		// Remove the outdated screenshot.
		DirAccess::remove_absolute(screenshot_path);
	}

	if (OS::get_singleton()->get_thread_caller_id() != OS::get_singleton()->get_main_thread_id()) {
		sentry::util::print_debug("skipping screenshot – can only be performed on the main thread");
		return p_event;
	}

	if (p_event->get_level() < SentryOptions::get_singleton()->get_screenshot_level()) {
		// This check needs to happen after we remove the outdated screenshot file from the drive.
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
			sentry::util::print_debug("cancelled screenshot: before_capture_screenshot returned false");
			return p_event;
		}
	}

	mutex.lock();
	last_screenshot_frame = current_frame;
	mutex.unlock();

	sentry::util::print_debug("taking screenshot");
	PackedByteArray buffer = sentry::util::take_screenshot();

	Ref<FileAccess> f = FileAccess::open(screenshot_path, FileAccess::WRITE);
	if (f.is_valid()) {
		f->store_buffer(buffer);
		f->flush();
		f->close();
	} else {
		sentry::util::print_error("failed to save ", screenshot_path);
	}

	return p_event;
}

ScreenshotProcessor::ScreenshotProcessor() {
	screenshot_path = "user://" SENTRY_SCREENSHOT_FN;
}

} // namespace sentry
