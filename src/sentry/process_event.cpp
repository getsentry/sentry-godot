#include "process_event.h"

#include "common_defs.h"
#include "sentry/contexts.h"
#include "sentry/util/print.h"
#include "sentry/util/screenshot.h"
#include "sentry_options.h"
#include "view_hierarchy.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>

namespace {

void _save_screenshot(const Ref<SentryEvent> &p_event) {
	if (!SentryOptions::get_singleton()->is_attach_screenshot_enabled()) {
		return;
	}

	static int32_t last_screenshot_frame = 0;
	int32_t current_frame = Engine::get_singleton()->get_frames_drawn();
	if (current_frame == last_screenshot_frame) {
		// Screenshot already exists for this frame — nothing to do.
		return;
	}
	last_screenshot_frame = current_frame;

	String screenshot_path = "user://" SENTRY_SCREENSHOT_FN;
	DirAccess::remove_absolute(screenshot_path);

	if (!DisplayServer::get_singleton() || DisplayServer::get_singleton()->get_name() == "headless") {
		return;
	}

	if (p_event->get_level() < SentryOptions::get_singleton()->get_screenshot_level()) {
		// This check needs to happen after we remove the outdated screenshot file from the drive.
		return;
	}

	if (SentryOptions::get_singleton()->get_before_capture_screenshot().is_valid()) {
		Variant result = SentryOptions::get_singleton()->get_before_capture_screenshot().call(p_event);
		if (result.get_type() != Variant::BOOL) {
			// Note: Using PRINT_ONCE to avoid feedback loop in case of error event.
			ERR_PRINT_ONCE("Sentry: before_capture_screenshot callback failed: expected a boolean return value");
			return;
		}
		if (result.operator bool() == false) {
			sentry::util::print_debug("cancelled screenshot: before_capture_screenshot returned false");
			return;
		}
	}

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
}

inline void _save_view_hierarchy() {
	if (!SentryOptions::get_singleton()->is_attach_scene_tree_enabled()) {
		return;
	}

#ifdef DEBUG_ENABLED
	uint64_t start = Time::get_singleton()->get_ticks_usec();
#endif

	String path = "user://" SENTRY_VIEW_HIERARCHY_FN;
	DirAccess::remove_absolute(path);

	if (OS::get_singleton()->get_thread_caller_id() != OS::get_singleton()->get_main_thread_id()) {
		sentry::util::print_debug("skipping scene tree capture - can only be performed on the main thread");
		return;
	}

	String json_content = sentry::build_view_hierarchy_json();
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
	if (f.is_valid()) {
		f->store_string(json_content);
		f->flush();
		f->close();
	} else {
		sentry::util::print_error("failed to save ", path);
	}

#ifdef DEBUG_ENABLED
	uint64_t end = Time::get_singleton()->get_ticks_usec();
	sentry::util::print_debug("capturing scene tree data took ", end - start, " usec");
#endif
}

inline void _inject_contexts(const Ref<SentryEvent> p_event) {
	HashMap<String, Dictionary> contexts = sentry::contexts::make_event_contexts();
	for (const auto &kv : contexts) {
		p_event->set_context(kv.key, kv.value);
	}
}

} // unnamed namespace

// *************

namespace sentry {

Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) {
	_save_screenshot(p_event);
	_save_view_hierarchy();
	_inject_contexts(p_event);

	if (const Callable &before_send = SentryOptions::get_singleton()->get_before_send(); before_send.is_valid()) {
		Ref<SentryEvent> processed = before_send.call(p_event);

		if (processed.is_valid() && processed != p_event) {
			static bool first_print = true;
			if (unlikely(first_print)) {
				// Note: Only push error once to avoid infinite feedback loop.
				ERR_PRINT("Sentry: before_send callback must return the same event object or null.");
				first_print = false;
			} else {
				sentry::util::print_error("before_send callback must return the same event object or null.");
			}
			return p_event;
		}

		if (processed.is_valid()) {
			sentry::util::print_debug("event processed by before_send callback: ", p_event->get_id());
		} else {
			sentry::util::print_debug("event discarded by before_send callback: ", p_event->get_id());
		}

		return processed;
	}

	return p_event;
}

} // namespace sentry
