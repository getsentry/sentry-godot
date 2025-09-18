#include "view_hierarchy_processor.h"

#include "sentry/common_defs.h"
#include "sentry/processing/view_hierarchy.h"
#include "sentry/util/print.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>

namespace sentry {

Ref<SentryEvent> ViewHierarchyProcessor::process_event(const Ref<SentryEvent> &p_event) {
#ifdef DEBUG_ENABLED
	uint64_t start = Time::get_singleton()->get_ticks_usec();
#endif

	String path = "user://" SENTRY_VIEW_HIERARCHY_FN;
	DirAccess::remove_absolute(path);

	if (OS::get_singleton()->get_thread_caller_id() != OS::get_singleton()->get_main_thread_id()) {
		sentry::util::print_debug("Skipping scene tree capture - can only be performed on the main thread");
		return p_event;
	}

	sentry::util::UTF8Buffer json_buffer = view_hierarchy_builder.build_json();

	FILE *f = std::fopen(json_file_path.ptr(), "wb");
	if (f) {
		size_t written = std::fwrite(json_buffer.ptr(), 1, json_buffer.get_used(), f);
		if (written != json_buffer.get_used()) {
			sentry::util::print_error(vformat("Failed to write scene tree data - only wrote %d bytes out of %d", (int64_t)written, (int64_t)json_buffer.get_used()));
		}
		std::fclose(f);
	} else {
		sentry::util::print_error(vformat("Failed to write scene tree data - unable to open file for writing: %s", json_file_path.get_data()));
	}

#ifdef DEBUG_ENABLED
	uint64_t end = Time::get_singleton()->get_ticks_usec();
	sentry::util::print_debug("Capturing scene tree data took ", end - start, " usec");
#endif

	return p_event;
}

ViewHierarchyProcessor::ViewHierarchyProcessor() {
	String path = "user://" SENTRY_VIEW_HIERARCHY_FN;
	ERR_FAIL_NULL(ProjectSettings::get_singleton());
	json_file_path = String(ProjectSettings::get_singleton()->globalize_path(path)).utf8();
}

} // namespace sentry
