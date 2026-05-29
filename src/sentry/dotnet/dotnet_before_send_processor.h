#pragma once

#include "sentry/processing/sentry_event_processor.h"

namespace sentry::dotnet {

// Forwards non-crash native events to the .NET BeforeSendGodot callback.
// Registered only when the .NET layer is present.
class DotnetBeforeSendProcessor : public SentryEventProcessor {
	GDCLASS(DotnetBeforeSendProcessor, SentryEventProcessor);

protected:
	static void _bind_methods() {}

public:
	virtual Ref<SentryEvent> process_event(const Ref<SentryEvent> &p_event) override;
};

} // namespace sentry::dotnet
