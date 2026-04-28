#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace sentry::editor {

// Patches csproj to import a props file (e.g. Sentry.Godot.props).
class CsprojPatcher {
private:
	enum class State {
		NORMAL,
		COMMENT,
		TAG_OPEN,
		TAG_SINGLE_QUOTES,
		TAG_DOUBLE_QUOTES,
		CDATA,
	};

public:
	enum class Status {
		PATCH_NOT_NEEDED,
		PATCHED,
		ERR_UNCLOSED_COMMENT,
		ERR_UNCLOSED_CDATA,
		ERR_UNCLOSED_TAG,
		ERR_NO_PROJECT_TAG,
		ERR_CONTENT_EMPTY,
		ERR_INVALID_PATH
	};

	struct Result {
		Status status;
		std::vector<std::uint8_t> patched_content;
	};

public:
	static Result ensure_import(const std::string_view p_csproj_content, std::string_view p_import_path);
};

} // namespace sentry::editor
