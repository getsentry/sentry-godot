#ifndef VIEW_HIERARCHY_PROCESSOR_H
#define VIEW_HIERARCHY_PROCESSOR_H

#include "sentry/processing/sentry_event_processor.h"

// Event processor for capturing the view hierarchy (aka scene tree state).
class ViewHierarchyProcessor : public SentryEventProcessor {
	GDCLASS(ViewHierarchyProcessor, SentryEventProcessor);

protected:
	static void _bind_methods() {}

public:
	virtual Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) override;
};

#endif // VIEW_HIERARCHY_PROCESSOR_H
