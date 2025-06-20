#include "sentry_editor_plugin.h"

#ifdef TOOLS_ENABLED

#include "editor/sentry_editor_export_plugin_unix.h"

#include "sentry/util/print.h"

void SentryEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
#ifndef WINDOWS_ENABLED
			if (unix_export_plugin.is_null()) {
				unix_export_plugin = Ref(memnew(SentryEditorExportPluginUnix));
			}
			sentry::util::print_debug("adding export plugin");
			add_export_plugin(unix_export_plugin);
#endif
		} break;
		case NOTIFICATION_EXIT_TREE: {
#ifndef WINDOWS_ENABLED
			if (unix_export_plugin.is_valid()) {
				remove_export_plugin(unix_export_plugin);
				unix_export_plugin.unref();
			}
#endif
		} break;
	}
}

#endif // TOOLS_ENABLED
