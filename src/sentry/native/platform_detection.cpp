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
	// - "9.0" (standard Wine or Proton Experimental)
	// - "9.0-4" (Proton)
	// - "7.0-rc1" (Wine release candidate)
	// - "9.0-GE" (Proton-GE)

	r_info.runtime_name = "Wine";

	int dash_pos = p_version.find("-");
	if (dash_pos < 0) {
		return;
	}

	String suffix = p_version.substr(dash_pos + 1);
	if (suffix.is_empty()) {
		return;
	}

	// Detect Proton or Proton-GE from Wine version suffix pattern.
	if (suffix.to_lower().begins_with("ge")) {
		r_info.is_proton = true;
		r_info.runtime_name = "Proton-GE";
	} else if (suffix.is_valid_int()) {
		r_info.is_proton = true;
		r_info.runtime_name = "Proton";
	} else {
		r_info.runtime_name = "Wine";
	}
}

sentry::native::WineProtonInfo _detect_wine_proton() {
	sentry::native::WineProtonInfo info;

	// Detect Wine via wine_get_version and wine_get_build_id exports from ntdll.dll.
	HMODULE h_ntdll = GetModuleHandleW(L"ntdll.dll");
	if (h_ntdll != nullptr) {
		typedef const char *(CDECL * wine_get_version_t)(void);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4191) // unsafe conversion from FARPROC
#endif
		wine_get_version_t wine_get_version =
				reinterpret_cast<wine_get_version_t>(GetProcAddress(h_ntdll, "wine_get_version"));
		wine_get_version_t wine_get_build_id =
				reinterpret_cast<wine_get_version_t>(GetProcAddress(h_ntdll, "wine_get_build_id"));
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		if (wine_get_version != nullptr) {
			const char *version = wine_get_version();
			info.is_wine = true;
			info.version = String::utf8(version);
			_parse_wine_version(info.version, info);

			sentry::logging::print_debug("Detected Wine version: ", info.version);
		}

		if (wine_get_build_id != nullptr) {
			info.build_id = String::utf8(wine_get_build_id());
			sentry::logging::print_debug("Wine build ID: ", info.build_id);
		}
	}

	// Detect Proton from Steam compatibility environment variables.
	if (info.is_wine) {
		String steam_compat_path = OS::get_singleton()->get_environment("STEAM_COMPAT_DATA_PATH");
		String steam_compat_install = OS::get_singleton()->get_environment("STEAM_COMPAT_CLIENT_INSTALL_PATH");

		if (!steam_compat_path.is_empty() || !steam_compat_install.is_empty()) {
			info.is_proton = true;
			if (info.runtime_name == "Wine") {
				info.runtime_name = "Proton";
			}
			sentry::logging::print_debug("Detected Proton environment.");
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
