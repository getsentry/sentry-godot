#ifndef SENTRY_EDITOR_PLUGIN_H
#define SENTRY_EDITOR_PLUGIN_H

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_export_plugin.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>

using namespace godot;

class SentryEditorPlugin : public EditorPlugin {
	GDCLASS(SentryEditorPlugin, EditorPlugin);

private:
	Ref<EditorExportPlugin> android_export_plugin;
	Ref<EditorExportPlugin> unix_export_plugin;
	Ref<EditorExportPlugin> web_export_plugin;

protected:
	static void _bind_methods() {}
	void _notification(int p_what);

public:
};

#endif // TOOLS_ENABLED

#endif // SENTRY_EDITOR_PLUGIN_H
