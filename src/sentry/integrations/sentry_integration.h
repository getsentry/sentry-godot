#pragma once

#include "sentry/castable.h"
#include "sentry/sentry_options.h"

namespace sentry {

// Internal lifecycle for SDK-owned instrumentation.
class SentryIntegration : public Castable {
	SENTRY_CASTABLE(SentryIntegration, Castable);

public:
	virtual const char *get_name() const = 0;

	virtual bool setup(const Ref<SentryOptions> &p_options) = 0;

	// Runs even if setup() returned false. Should be idempotent.
	virtual void teardown() = 0;

	~SentryIntegration() override = default;
};

} //namespace sentry
