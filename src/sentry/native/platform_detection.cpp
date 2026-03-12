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
void _parse_wine_version(const String &p_version, sentry::native::WineProtonInfo &r_info) {
	// Wine versions typically look like:
	// - "9.0" (standard Wine)
	// - "8.0-3" (Proton)
	// - "7.0-rc1" (Wine release candidate)
	// - "8.0-GE" (Proton-GE)

	int dash_pos = p_version.find("-");
	if (dash_pos < 0) {
		return;
	}

	String suffix = p_version.substr(dash_pos + 1);
	if (suffix.is_empty()) {
		return;
	}

	// Check if the part after dash is numeric (Proton style) or GE build.
	if (suffix.is_valid_int() || suffix.to_lower() == "ge") {
		// TODO: Not sure we can reliably detect Proton based on number heuristics - need to verify this.
		r_info.is_proton = true;

		if (r_info.proton_version.is_empty()) {
			if (suffix.to_lower() == "ge") {
				r_info.proton_version = "Proton-GE " + p_version;
			} else {
				r_info.proton_version = "Proton " + p_version.substr(0, dash_pos);
			}
		}
	}
}

sentry::native::WineProtonInfo _detect_wine_proton() {
	// NOTE: Adapted from Unreal SDK implemntation.

	sentry::native::WineProtonInfo info;

	// Detect Wine from ntdll.dll.
	HMODULE h_ntdll = GetModuleHandleW(L"ntdll.dll");
	if (h_ntdll != nullptr) {
		// wine_get_version is exported by Wine's ntdll.
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
			info.is_wine = true;
			info.wine_version = String::utf8(version);
			_parse_wine_version(info.wine_version, info);

			sentry::logging::print_debug("Detected Wine version: ", info.wine_version);
		}
	}

	// Fallback: Detect Wine from environment variables.
	if (!info.is_wine) {
		String wine_version = OS::get_singleton()->get_environment("WINE_VERSION");
		if (!wine_version.is_empty()) {
			info.is_wine = true;
			info.wine_version = wine_version;
			_parse_wine_version(info.wine_version, info);

			sentry::logging::print_debug("Detected Wine/Proton via WINE_VERSION environment variable: ", wine_version);
		}
	}

	// Detect Proton from expected environment variables.
	if (info.is_wine) {
		String steam_compat_path = OS::get_singleton()->get_environment("STEAM_COMPAT_DATA_PATH");
		String steam_compat_install = OS::get_singleton()->get_environment("STEAM_COMPAT_CLIENT_INSTALL_PATH");

		if (!steam_compat_path.is_empty() || !steam_compat_install.is_empty()) {
			info.is_proton = true;
			sentry::logging::print_debug("Detected Proton environment.");
		}

		// Try to get Proton build name from PROTON_VERSION environment variable.
		String proton_version = OS::get_singleton()->get_environment("PROTON_VERSION");
		if (!proton_version.is_empty()) {
			info.proton_version = proton_version;
			info.is_experimental = proton_version.contains("Experimental");
		}
	}

	return info;
}
#endif // WINDOWS_ENABLED

// Parses /etc/os-release into a key-value map.
// Format: KEY=VALUE or KEY="VALUE" (one per line).
HashMap<String, String> _parse_os_release() {
	HashMap<String, String> fields;
	// On native Linux, read directly. Under Wine/Proton, Z:\ maps to the host root.
	Ref<FileAccess> f = FileAccess::open("/etc/os-release", FileAccess::READ);
#ifdef WINDOWS_ENABLED
	if (!f.is_valid()) {
		// Typically, Wine mounts the host root at Z:\.
		f = FileAccess::open("Z:/etc/os-release", FileAccess::READ);
	}
#endif
	if (!f.is_valid()) {
		return fields;
	}

	while (!f->eof_reached()) {
		String line = f->get_line().strip_edges();
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

#ifdef WINDOWS_ENABLED
		cached_info.wine_proton = _detect_wine_proton();
#endif
		HashMap<String, String> os_release = _parse_os_release();
		cached_info.is_steamos = _detect_steamos(os_release);
		cached_info.is_bazzite = _detect_bazzite(os_release);
		cached_info.is_steam = _detect_steam();

		// Populate DistroInfo.
		if (os_release.has("PRETTY_NAME")) {
			cached_info.distro.name = os_release["PRETTY_NAME"];
		} else if (os_release.has("NAME")) {
			cached_info.distro.name = os_release["NAME"];
		}

		if (os_release.has("VERSION")) {
			cached_info.distro.version = os_release["VERSION"];
		} else if (os_release.has("VERSION_ID")) {
			cached_info.distro.version = os_release["VERSION_ID"];
		}

		if (os_release.has("VERSION_CODENAME")) {
			cached_info.distro.codename = os_release["VERSION_CODENAME"];
		}

		if (os_release.has("BUILD_ID")) {
			cached_info.distro.build = os_release["BUILD_ID"];
		}

		if (os_release.has("VARIANT_ID")) {
			cached_info.distro.variant = os_release["VARIANT_ID"];
		}

		if (os_release.has("IMAGE_ID")) {
			cached_info.distro.image_id = os_release["IMAGE_ID"];
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
