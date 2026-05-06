#pragma once

namespace sentry {

// Runs Sentry project settings migrations, updating schema version and migrating legacy settings.
// This function should be called before Sentry-related settings are defined (see sentry_options.cpp).
void run_project_settings_migrations();

} //namespace sentry
