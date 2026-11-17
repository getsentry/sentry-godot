#include "sentry_editor_plugin.h"

#ifdef TOOLS_ENABLED

#include "editor/sentry_editor_export_plugin_android.h"
#include "editor/sentry_editor_export_plugin_ios.h"
#include "editor/sentry_editor_export_plugin_macos.h"
#include "editor/sentry_editor_export_plugin_native.h"
#include "editor/sentry_editor_export_plugin_unix.h"
#include "editor/sentry_editor_export_plugin_web.h"
#include "sentry/logging/print.h"

void SentryEditorPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			sentry::logging::print_debug("adding export plugins");

			if (_native_export_plugin.is_null()) {
				_native_export_plugin = Ref(memnew(SentryEditorExportPluginNative));
			}
			add_export_plugin(_native_export_plugin);

			if (_android_export_plugin.is_null()) {
				_android_export_plugin = Ref(memnew(SentryEditorExportPluginAndroid));
			}
			add_export_plugin(_android_export_plugin);

#ifndef WINDOWS_ENABLED
			if (_unix_export_plugin.is_null()) {
				_unix_export_plugin = Ref(memnew(SentryEditorExportPluginUnix));
			}
			add_export_plugin(_unix_export_plugin);
#endif

			if (_web_export_plugin.is_null()) {
				_web_export_plugin = Ref(memnew(SentryEditorExportPluginWeb));
			}
			add_export_plugin(_web_export_plugin);

			if (_ios_export_plugin.is_null()) {
				_ios_export_plugin = Ref(memnew(SentryEditorExportPluginIOS));
			}
			add_export_plugin(_ios_export_plugin);

			if (_macos_export_plugin.is_null()) {
				_macos_export_plugin = Ref(memnew(SentryEditorExportPluginMacOS));
			}
			add_export_plugin(_macos_export_plugin);
		} break;

		case NOTIFICATION_EXIT_TREE: {
			if (_native_export_plugin.is_valid()) {
				remove_export_plugin(_native_export_plugin);
				_native_export_plugin.unref();
			}

			if (_android_export_plugin.is_valid()) {
				remove_export_plugin(_android_export_plugin);
				_android_export_plugin.unref();
			}

#ifndef WINDOWS_ENABLED
			if (_unix_export_plugin.is_valid()) {
				remove_export_plugin(_unix_export_plugin);
				_unix_export_plugin.unref();
			}
#endif

			if (_web_export_plugin.is_valid()) {
				remove_export_plugin(_web_export_plugin);
				_web_export_plugin.unref();
			}

			if (_ios_export_plugin.is_valid()) {
				remove_export_plugin(_ios_export_plugin);
				_ios_export_plugin.unref();
			}

			if (_macos_export_plugin.is_valid()) {
				remove_export_plugin(_macos_export_plugin);
				_macos_export_plugin.unref();
			}
		} break;
	}
}

#endif // TOOLS_ENABLED
