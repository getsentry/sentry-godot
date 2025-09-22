#ifndef VIEW_HIERARCHY_PROCESSOR_H
#define VIEW_HIERARCHY_PROCESSOR_H

#include "sentry/processing/sentry_event_processor.h"
#include "sentry/processing/view_hierarchy_builder.h"

#include <godot_cpp/variant/char_string.hpp>

namespace sentry {

// Event processor for capturing the view hierarchy (aka scene tree state).
class ViewHierarchyProcessor : public SentryEventProcessor {
	GDCLASS(ViewHierarchyProcessor, SentryEventProcessor);

private:
	CharString json_file_path;
	ViewHierarchyBuilder view_hierarchy_builder;

protected:
	static void _bind_methods() {}

public:
	virtual Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) override;

	ViewHierarchyProcessor();
};

} // namespace sentry

#endif // VIEW_HIERARCHY_PROCESSOR_H
