#include "sentry_editor_plugin.h"

#ifdef TOOLS_ENABLED

#include "editor/sentry_editor_export_plugin_android.h"
#include "editor/sentry_editor_export_plugin_unix.h"
#include "editor/sentry_editor_export_plugin_web.h"
#include "sentry/logging/print.h"

void SentryEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			sentry::logging::print_debug("adding export plugins");

			if (android_export_plugin.is_null()) {
				android_export_plugin = Ref(memnew(SentryEditorExportPluginAndroid));
			}
			add_export_plugin(android_export_plugin);

#ifndef WINDOWS_ENABLED
			if (unix_export_plugin.is_null()) {
				unix_export_plugin = Ref(memnew(SentryEditorExportPluginUnix));
			}
			add_export_plugin(unix_export_plugin);
#endif

			if (web_export_plugin.is_null()) {
				web_export_plugin = Ref(memnew(SentryEditorExportPluginWeb));
			}
			add_export_plugin(web_export_plugin);
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (android_export_plugin.is_valid()) {
				remove_export_plugin(android_export_plugin);
				android_export_plugin.unref();
			}

#ifndef WINDOWS_ENABLED
			if (unix_export_plugin.is_valid()) {
				remove_export_plugin(unix_export_plugin);
				unix_export_plugin.unref();
			}
#endif

			if (web_export_plugin.is_valid()) {
				remove_export_plugin(web_export_plugin);
				web_export_plugin.unref();
			}
		} break;
	}
}

#endif // TOOLS_ENABLED
