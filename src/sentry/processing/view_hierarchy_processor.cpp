#include "view_hierarchy_processor.h"

#include "sentry/common_defs.h"
#include "sentry/util/print.h"
#include "sentry/view_hierarchy.h" // TODO: incorporate

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/time.hpp>

Ref<SentryEvent> ViewHierarchyProcessor::process_event(const Ref<SentryEvent> &p_event) {
#ifdef DEBUG_ENABLED
	uint64_t start = Time::get_singleton()->get_ticks_usec();
#endif

	String path = "user://" SENTRY_VIEW_HIERARCHY_FN;
	DirAccess::remove_absolute(path);

	if (OS::get_singleton()->get_thread_caller_id() != OS::get_singleton()->get_main_thread_id()) {
		sentry::util::print_debug("skipping scene tree capture - can only be performed on the main thread");
		return p_event;
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

	return p_event;
}
