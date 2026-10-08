#pragma once

#include "sentry/integrations/sentry_integration.h"

#include <godot_cpp/core/object_id.hpp>

namespace sentry {

class AutoMetricsIntegration final : public SentryIntegration {
	SENTRY_CASTABLE(AutoMetricsIntegration, SentryIntegration);

private:
	ObjectID _runner_id;

public:
	const char *get_name() const override { return "Auto Metrics"; }

	bool setup(const Ref<SentryOptions> &p_options) override;
	void teardown() override;
};

} //namespace sentry
