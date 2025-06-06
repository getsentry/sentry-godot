#include "sentry_editor_plugin.h"

#ifdef TOOLS_ENABLED

#include "sentry/util/print.h"
#include "sentry_editor_export_plugin.h"

void SentryEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (export_plugin.is_null()) {
				export_plugin = Ref(memnew(SentryEditorExportPlugin));
			}
			sentry::util::print_debug("adding export plugin");
			add_export_plugin(export_plugin);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (export_plugin.is_valid()) {
				remove_export_plugin(export_plugin);
				export_plugin.unref();
			}
		} break;
	}
}

#endif // TOOLS_ENABLED
