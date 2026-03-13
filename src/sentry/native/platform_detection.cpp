#include "platform_detection.h"

#include "sentry/logging/print.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#ifdef WINDOWS_ENABLED
#include <windows.h>
#endif

namespace {

#ifdef WINDOWS_ENABLED

void _detect_wine(sentry::native::WineProtonInfo &r_info) {
	// Detect Wine via wine_get_version export from ntdll.dll.
	HMODULE h_ntdll = GetModuleHandleW(L"ntdll.dll");
	if (h_ntdll != nullptr) {
		typedef const char *(CDECL * wine_get_version_t)(void);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4191) // unsafe conversion from FARPROC
#endif
		wine_get_version_t wine_get_version =
				reinterpret_cast<wine_get_version_t>(GetProcAddress(h_ntdll, "wine_get_version"));
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		if (wine_get_version != nullptr) {
			const char *version = wine_get_version();
			if (version == nullptr) {
				version = "";
			}
			r_info.is_wine = true;
			r_info.runtime_name = "Wine";
			r_info.version = String::utf8(version);

			sentry::logging::print_debug("Detected Wine version: ", r_info.version);
		}
	}
}

// Detects the precise Proton version from the filesystem.
// Approach: STEAM_COMPAT_DATA_PATH/config_info contains paths to the Proton install directory.
// The Proton install directory contains a "version" file with format: "<timestamp> <git-tag>".
void _detect_proton(const String &p_steam_compat_path, sentry::native::WineProtonInfo &r_info) {
	if (p_steam_compat_path.is_empty()) {
		return;
	}

	// Read config_info to find the Proton install directory.
	Ref<FileAccess> f = FileAccess::open("Z:" + p_steam_compat_path + "/config_info", FileAccess::READ);
	if (!f.is_valid()) {
		return;
	}

	f->get_line(); // Skip line 1 (CURRENT_PREFIX_VERSION).
	String fonts_dir = f->get_line().strip_edges(); // Line 2: Proton fonts directory path.
	f.unref();

	// Extract Proton root by stripping the known suffix.
	// Modern Proton uses "files/", older versions used "dist/".
	String proton_root;
	if (fonts_dir.ends_with("/files/share/fonts/")) {
		proton_root = fonts_dir.trim_suffix("/files/share/fonts/");
	} else if (fonts_dir.ends_with("/dist/share/fonts/")) {
		proton_root = fonts_dir.trim_suffix("/dist/share/fonts/");
	} else {
		sentry::logging::print_debug("Unexpected config_info fonts path: ", fonts_dir);
		return;
	}

	// Read the version file from the compatibility tool install directory.
	Ref<FileAccess> vf = FileAccess::open("Z:" + proton_root + "/version", FileAccess::READ);
	if (!vf.is_valid()) {
		return;
	}
	String version_line = vf->get_line().strip_edges();
	vf.unref();

	// Format: "<timestamp> <git-tag>", e.g., "1769167055 proton-10.0-4".
	String git_tag = version_line.get_slice(" ", 1).strip_edges();
	if (git_tag.is_empty()) {
		return;
	}

	// Determine the runtime name and version.
	if (FileAccess::file_exists("Z:" + proton_root + "/proton")) {
		r_info.is_proton = true;
		if (git_tag.begins_with("proton-")) {
			// proton-10.0-4 -> "Proton" "10.0-4"
			r_info.runtime_name = "Proton";
			r_info.version = git_tag.trim_prefix("proton-");
		} else if (git_tag.begins_with("experimental-")) {
			// experimental-10.0-20260113 -> "Proton Experimental" "10.0-20260113"
			r_info.runtime_name = "Proton Experimental";
			r_info.version = git_tag.trim_prefix("experimental-");
		} else if (git_tag.begins_with("GE-Proton")) {
			// GE-Proton9-27 -> "GE-Proton" "9-27"
			r_info.runtime_name = "GE-Proton";
			r_info.version = git_tag.trim_prefix("GE-Proton");
		} else if (git_tag.begins_with("hotfix-")) {
			// hotfix-20251031 -> "Proton Hotfix" "20251031"
			r_info.runtime_name = "Proton Hotfix";
			r_info.version = git_tag.trim_prefix("hotfix-");
		} else {
			// Unrecognized Proton-like tool.
			r_info.runtime_name = "Proton Custom";
			r_info.version = git_tag;
		}
	} else {
		// Other Wine-like tool: tag might still be more informative.
		r_info.version = git_tag;
	}

	sentry::logging::print_debug("Detected Proton version: ", r_info.runtime_name, " ", r_info.version);
}

sentry::native::WineProtonInfo _detect_wine_proton() {
	sentry::native::WineProtonInfo info;

	_detect_wine(info);

	if (info.is_wine) {
		String steam_compat_path = OS::get_singleton()->get_environment("STEAM_COMPAT_DATA_PATH");
		if (!steam_compat_path.is_empty()) {
			sentry::logging::print_debug("Detected Steam compatibility environment.");
			_detect_proton(steam_compat_path, info);
		}
	}

	return info;
}
#endif // WINDOWS_ENABLED

// Parses /etc/os-release into a DistroInfo struct.
// Format: KEY=VALUE or KEY="VALUE" (one per line).
sentry::native::DistroInfo _read_distro_info() {
	sentry::native::DistroInfo distro;

#ifdef WINDOWS_ENABLED
	// Under Wine/Proton, Z:\ maps to the container/host root.
	// Prefer /run/host/etc/os-release (host OS) over /etc/os-release (Steam Runtime container).
	Ref<FileAccess> f = FileAccess::open("Z:/run/host/etc/os-release", FileAccess::READ);
	if (!f.is_valid()) {
		f = FileAccess::open("Z:/etc/os-release", FileAccess::READ);
	}
#else
	// On native Linux, prefer host os-release (in case of Steam Runtime container).
	Ref<FileAccess> f = FileAccess::open("/run/host/etc/os-release", FileAccess::READ);
	if (!f.is_valid()) {
		f = FileAccess::open("/etc/os-release", FileAccess::READ);
	}
#endif
	if (!f.is_valid()) {
		return distro;
	}

	// Parse os-release file.
	HashMap<String, String> fields;
	while (!f->eof_reached()) {
		String line = f->get_line().strip_edges();
		if (line.is_empty() || line.begins_with("#")) {
			continue;
		}
		int eq_pos = line.find("=");
		if (eq_pos < 0) {
			continue;
		}

		String key = line.substr(0, eq_pos);
		String value = line.substr(eq_pos + 1);

		// Trim enclosing quotes.
		if (value.length() >= 2 &&
				((value.begins_with("\"") && value.ends_with("\"")) ||
						(value.begins_with("'") && value.ends_with("'")))) {
			value = value.substr(1, value.length() - 2);
		}

		fields[key] = value;
	}

	// Populate struct.
	if (fields.has("NAME")) {
		distro.name = fields["NAME"];
	}
	if (fields.has("PRETTY_NAME")) {
		distro.pretty_name = fields["PRETTY_NAME"];
	}
	if (fields.has("VERSION")) {
		distro.version = fields["VERSION"];
	} else if (fields.has("VERSION_ID")) {
		distro.version = fields["VERSION_ID"];
	}
	if (fields.has("BUILD_ID")) {
		distro.build = fields["BUILD_ID"];
	}
	if (fields.has("VARIANT_ID")) {
		distro.variant = fields["VARIANT_ID"];
	}
	// Update/release branch:
	// - SteamOS uses STEAMOS_DEFAULT_UPDATE_BRANCH
	// - Bazzite uses RELEASE_TYPE
	if (fields.has("STEAMOS_DEFAULT_UPDATE_BRANCH")) {
		distro.update_branch = fields["STEAMOS_DEFAULT_UPDATE_BRANCH"];
	} else if (fields.has("RELEASE_TYPE")) {
		distro.update_branch = fields["RELEASE_TYPE"];
	}

	return distro;
}

bool _detect_steamos(const sentry::native::DistroInfo &p_distro) {
	if (p_distro.name.to_lower() == "steamos") {
		sentry::logging::print_debug("Detected SteamOS via os-release.");
		return true;
	}

	// Fallback: Check environment variables.
	String steamos_var = OS::get_singleton()->get_environment("SteamOS");
	if (!steamos_var.is_empty()) {
		sentry::logging::print_debug("Detected SteamOS via SteamOS environment variable.");
		return true;
	}

	return false;
}

bool _detect_bazzite(const sentry::native::DistroInfo &p_distro) {
	if (p_distro.name.to_lower() == "bazzite") {
		sentry::logging::print_debug("Detected Bazzite via os-release");
		return true;
	}

	return false;
}

String _read_kernel_version() {
#ifdef WINDOWS_ENABLED
	Ref<FileAccess> f = FileAccess::open("Z:/proc/sys/kernel/osrelease", FileAccess::READ);
#else
	Ref<FileAccess> f = FileAccess::open("/proc/sys/kernel/osrelease", FileAccess::READ);
#endif
	if (!f.is_valid()) {
		return String();
	}
	return f->get_line().strip_edges();
}

bool _detect_steam() {
	String steam_app_id = OS::get_singleton()->get_environment("SteamAppId");
	String steam_game_id = OS::get_singleton()->get_environment("SteamGameId");
	String steam_overlay_id = OS::get_singleton()->get_environment("SteamOverlayGameId");

	return !steam_app_id.is_empty() || !steam_game_id.is_empty() || !steam_overlay_id.is_empty();
}

} // unnamed namespace

namespace sentry::native {

const PlatformInfo &detect_platform() {
	static PlatformInfo cached_info;
	static bool first_run = true;

	if (first_run) {
		first_run = false;

#ifdef LINUX_ENABLED
		cached_info.distro = _read_distro_info();
		cached_info.kernel_version = _read_kernel_version();
#elif defined(WINDOWS_ENABLED)
		cached_info.wine_proton = _detect_wine_proton();
		if (cached_info.wine_proton.is_wine) {
			cached_info.distro = _read_distro_info();
			cached_info.kernel_version = _read_kernel_version();
		}
#endif

		cached_info.is_steamos = _detect_steamos(cached_info.distro);
		cached_info.is_bazzite = _detect_bazzite(cached_info.distro);
		cached_info.is_steam = _detect_steam();
	}

	return cached_info;
}

} // namespace sentry::native
