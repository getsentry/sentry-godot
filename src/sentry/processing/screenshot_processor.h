#ifndef SCREENSHOT_EVENT_PROCESSOR_H
#define SCREENSHOT_EVENT_PROCESSOR_H

#include "sentry/processing/sentry_event_processor.h"

#include <mutex>

namespace sentry {

// Event processor for capturing in-engine screenshots.
class ScreenshotProcessor : public SentryEventProcessor {
	GDCLASS(ScreenshotProcessor, SentryEventProcessor);

private:
	String screenshot_path;
	int32_t last_screenshot_frame = 0;
	std::mutex mutex;

protected:
	static void _bind_methods() {}

public:
	virtual Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) override;

	ScreenshotProcessor();
};

} // namespace sentry

#endif // SCREENSHOT_EVENT_PROCESSOR_H
