#include "dotnet_tools.h"

#ifdef TOOLS_ENABLED

#include "editor/csproj_patcher.h"
#include "sentry/logging/print.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace {

constexpr const char *PROPS_PATH = "addons/sentry/dotnet/Sentry.Godot.props";

void _patch_csproj() {
	String assembly_name = ProjectSettings::get_singleton()->get_setting("dotnet/project/assembly_name");
	if (assembly_name.is_empty()) {
		// No .NET project here.
		sentry::logging::print_debug(".NET assembly name not detected - skipped C# project patching.");
		return;
	}

	String csproj_path = "res://" + assembly_name + ".csproj";
	PackedByteArray content = FileAccess::get_file_as_bytes(csproj_path);
	if (FileAccess::get_open_error() != OK) {
		WARN_PRINT_ED("Failed to read C# project file - skipped patching. Try restarting Godot editor.");
		return;
	}

	if (content.is_empty()) {
		WARN_PRINT_ED("C# project file is empty - skipped patching. Try restarting Godot editor.");
		return;
	}

	sentry::editor::CsprojPatcher::Result result = sentry::editor::CsprojPatcher::ensure_import(
			{ (const char *)content.ptr(), (size_t)content.size() },
			PROPS_PATH);

	if (result.status == sentry::editor::CsprojPatcher::Status::PATCHED) {
		sentry::logging::print_debug(".NET assembly detected: patching C# project file...");
		String temp_path = csproj_path + ".tmp";
		Ref<FileAccess> f = FileAccess::open(temp_path, FileAccess::WRITE);
		if (f.is_null()) {
			ERR_PRINT_ED("Failed to write temporary C# project file.");
			return;
		}
		f->store_buffer(result.patched_content.data(), result.patched_content.size());
		f->close();

		Error err = DirAccess::rename_absolute(temp_path, csproj_path);
		if (err != OK) {
			ERR_PRINT_ED("Failed to replace C# project file with patched content.");
			DirAccess::remove_absolute(temp_path);
		}
	} else if (result.status == sentry::editor::CsprojPatcher::Status::ERROR) {
		ERR_PRINT_ED("Failed to patch C# project file: " + String(result.error_message.data()));
	} else if (result.status == sentry::editor::CsprojPatcher::Status::PATCH_NOT_NEEDED) {
		sentry::logging::print_debug(".NET assembly detected: C# project file already patched - skipped.");
	}
}

} // unnamed namespace

namespace editor {

void prepare_csharp_project() {
	_patch_csproj();
}

} //namespace editor

#endif // TOOLS_ENABLED
