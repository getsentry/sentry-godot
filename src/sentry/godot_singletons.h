#pragma once

namespace sentry::godot_singletons {

// Marks engine singletons as ready for access. Should be called when singletons are fully initialized.
void mark_as_ready();

// Checks whether engine singletons are fully initialized and safe to access.
bool are_ready();

} // namespace sentry::godot_singletons
