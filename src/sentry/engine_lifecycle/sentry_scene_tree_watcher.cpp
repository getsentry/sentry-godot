#include "sentry_scene_tree_watcher.h"

namespace sentry::engine_lifecycle {

void SentrySceneTreeWatcher::_notification(int p_what) {
	if (p_what == NOTIFICATION_PREDELETE && _shutdown_callback) {
		_shutdown_callback();
	}
}

} //namespace sentry::engine_lifecycle
