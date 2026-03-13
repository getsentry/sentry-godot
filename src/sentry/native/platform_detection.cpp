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

// Reads the precise Proton version from the filesystem.
// Approach: STEAM_COMPAT_DATA_PATH/config_info contains paths to the Proton install directory.
// The Proton install directory contains a "version" file with format: "<timestamp> <git-tag>".
void _read_proton_version(const String &p_steam_compat_path, sentry::native::WineProtonInfo &r_info) {
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

	// If this is a Proton-like tool (contains "proton" script),
	// determine the runtime name and version.
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
			// Unknown Proton-like tool -> use directory name.
			r_info.runtime_name = proton_root.get_file();
			r_info.version = git_tag;
		}
	} else {
		// Wine-like tool: tag might still be more informative.
		r_info.version = git_tag;
	}

	sentry::logging::print_debug("Proton version detected from version file: ", r_info.runtime_name, " ", r_info.version);
}

sentry::native::WineProtonInfo _detect_wine_proton() {
	sentry::native::WineProtonInfo info;

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
			info.is_wine = true;
			info.runtime_name = "Wine";
			info.version = String::utf8(version);

			sentry::logging::print_debug("Detected Wine version: ", info.version);
		}
	}

	// Detect Proton from Steam compatibility environment variables.
	if (info.is_wine) {
		String steam_compat_path = OS::get_singleton()->get_environment("STEAM_COMPAT_DATA_PATH");
		String steam_compat_install = OS::get_singleton()->get_environment("STEAM_COMPAT_CLIENT_INSTALL_PATH");

		if (!steam_compat_path.is_empty() || !steam_compat_install.is_empty()) {
			sentry::logging::print_debug("Detected Steam compatibility environment.");
			// Try to read precise Proton version from filesystem.
			// Proton writes config_info which contains paths embedding the install directory.
			// From there, we can read the version file with the exact git tag (e.g. "proton-9.0-4").
			_read_proton_version(steam_compat_path, info);
		}
	}

	return info;
}
#endif // WINDOWS_ENABLED

// Parses /etc/os-release into a key-value map.
// Format: KEY=VALUE or KEY="VALUE" (one per line).
HashMap<String, String> _parse_os_release() {
	HashMap<String, String> fields;

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
		return fields;
	}

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

		// Trim enclosing quotes or single quotes.
		if (value.length() >= 2 &&
				((value.begins_with("\"") && value.ends_with("\"")) ||
						(value.begins_with("'") && value.ends_with("'")))) {
			value = value.substr(1, value.length() - 2);
		}

		fields[key] = value;
	}

	return fields;
}

bool _detect_steamos(const HashMap<String, String> &p_os_release) {
	// Primary: Check /etc/os-release.
	if (p_os_release.has("ID") && p_os_release["ID"].to_lower() == "steamos") {
		sentry::logging::print_debug("Detected SteamOS via /etc/os-release");
		return true;
	}

	// Fallback: Check environment variables.
	String steamos_var = OS::get_singleton()->get_environment("SteamOS");
	if (!steamos_var.is_empty()) {
		sentry::logging::print_debug("Detected SteamOS via SteamOS environment variable");
		return true;
	}

	return false;
}

bool _detect_bazzite(const HashMap<String, String> &p_os_release) {
	// Primary: Check /etc/os-release.
	if (p_os_release.has("ID") && p_os_release["ID"].to_lower() == "bazzite") {
		sentry::logging::print_debug("Detected Bazzite via /etc/os-release");
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

		HashMap<String, String> os_release;
#ifdef LINUX_ENABLED
		os_release = _parse_os_release();
		cached_info.kernel_version = _read_kernel_version();
#elif defined(WINDOWS_ENABLED)
		cached_info.wine_proton = _detect_wine_proton();
		if (cached_info.wine_proton.is_wine) {
			os_release = _parse_os_release();
			cached_info.kernel_version = _read_kernel_version();
		}
#endif
		cached_info.is_steamos = _detect_steamos(os_release);
		cached_info.is_bazzite = _detect_bazzite(os_release);
		cached_info.is_steam = _detect_steam();

		// Populate DistroInfo.
		if (os_release.has("NAME")) {
			cached_info.distro.name = os_release["NAME"];
		}
		if (os_release.has("PRETTY_NAME")) {
			cached_info.distro.pretty_name = os_release["PRETTY_NAME"];
		}
		if (os_release.has("VERSION")) {
			cached_info.distro.version = os_release["VERSION"];
		} else if (os_release.has("VERSION_ID")) {
			cached_info.distro.version = os_release["VERSION_ID"];
		}
		if (os_release.has("BUILD_ID")) {
			cached_info.distro.build = os_release["BUILD_ID"];
		}
		if (os_release.has("VARIANT_ID")) {
			cached_info.distro.variant = os_release["VARIANT_ID"];
		}
		// Update/release branch:
		// - SteamOS uses STEAMOS_DEFAULT_UPDATE_BRANCH
		// - Bazzite uses RELEASE_TYPE
		if (os_release.has("STEAMOS_DEFAULT_UPDATE_BRANCH")) {
			cached_info.distro.update_branch = os_release["STEAMOS_DEFAULT_UPDATE_BRANCH"];
		} else if (os_release.has("RELEASE_TYPE")) {
			cached_info.distro.update_branch = os_release["RELEASE_TYPE"];
		}
	}
	return cached_info;
}

} // namespace sentry::native
