#pragma once

#include "sentry/integrations/godot_logger/sentry_godot_logger.h"
#include "sentry/integrations/sentry_integration.h"

namespace sentry {

class GodotLoggerIntegration : public SentryIntegration {
	SENTRY_CASTABLE(GodotLoggerIntegration, SentryIntegration);

private:
	Ref<::sentry::logging::SentryGodotLogger> _logger;

public:
	const char *get_name() const override { return "Godot Logger"; }

	bool setup(const Ref<SentryOptions> &p_options) override;
	void teardown() override;
};

} //namespace sentry
