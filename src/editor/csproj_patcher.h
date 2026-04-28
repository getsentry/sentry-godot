#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace sentry::editor {

// Patches csproj to import a props file (e.g. Sentry.Godot.props).
class CsprojPatcher {
public:
	enum class Status {
		PATCH_NOT_NEEDED,
		PATCHED,
		ERROR
	};

	struct Result {
		Status status;
		std::vector<std::uint8_t> patched_content;
		std::string_view error_message;
	};

public:
	// Adds an import to the csproj content if it is not already present.
	// p_import_path must not contain XML-special characters (&, <, >, ", ').
	// Caller is responsible for any necessary escaping.
	static Result ensure_import(const std::string_view p_csproj_content, std::string_view p_import_path);
};

} // namespace sentry::editor
