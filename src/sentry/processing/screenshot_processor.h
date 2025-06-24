#ifndef SCREENSHOT_EVENT_PROCESSOR_H
#define SCREENSHOT_EVENT_PROCESSOR_H

#include "sentry_event_processor.h"

// Event processor for capturing in-engine screenshots.
class ScreenshotProcessor : public SentryEventProcessor {
	GDCLASS(ScreenshotProcessor, SentryEventProcessor);

private:
	int32_t last_screenshot_frame = 0;

protected:
	static void _bind_methods() {}

public:
	virtual Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) override;
};

#endif // SCREENSHOT_EVENT_PROCESSOR_H
