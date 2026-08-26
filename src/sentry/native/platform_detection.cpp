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

bool _detect_wine() {
	HMODULE h_ntdll = GetModuleHandleW(L"ntdll.dll");
	if (h_ntdll == nullptr) {
		return false;
	}

	const bool is_wine = GetProcAddress(h_ntdll, "wine_get_version") != nullptr;
	if (is_wine) {
		sentry::logging::print_debug("Detected Wine/Proton compatibility environment.");
	}
	return is_wine;
}

// Read a REG_SZ string value from an open registry key.
String _read_registry_string(HKEY p_key, LPCWSTR p_value_name) {
	WCHAR buffer[256];
	DWORD buffer_len = sizeof(buffer);
	DWORD vtype = REG_SZ;
	if (RegQueryValueExW(p_key, p_value_name, nullptr, &vtype, (LPBYTE)buffer, &buffer_len) == ERROR_SUCCESS && buffer_len > sizeof(WCHAR)) {
		int64_t len = (buffer_len / sizeof(WCHAR)) - 1; // exclude null terminator
		return String::utf16((const char16_t *)buffer, len).strip_edges();
	}
	return String();
}

#endif // WINDOWS_ENABLED

String _read_rootfs_file(const String &p_rootfs_path) {
	ERR_FAIL_COND_V(!p_rootfs_path.begins_with("/"), String());

#ifdef WINDOWS_ENABLED
	Ref<FileAccess> f = FileAccess::open("Z:/run/host" + p_rootfs_path, FileAccess::READ);
	if (!f.is_valid()) {
		f = FileAccess::open("Z:" + p_rootfs_path, FileAccess::READ);
	}
#else
	Ref<FileAccess> f = FileAccess::open(p_rootfs_path, FileAccess::READ);
#endif

	if (!f.is_valid()) {
		return String();
	}
	return f->get_line().strip_edges();
}

// See https://www.dmtf.org/standards/smbios
sentry::native::ProductInfo _read_product_info() {
	sentry::native::ProductInfo product;

#ifdef LINUX_ENABLED
	product.name = _read_rootfs_file("/sys/class/dmi/id/product_name");
	product.family = _read_rootfs_file("/sys/class/dmi/id/product_family");
	product.manufacturer = _read_rootfs_file("/sys/class/dmi/id/sys_vendor");
	product.board_name = _read_rootfs_file("/sys/class/dmi/id/board_name");
#endif // LINUX_ENABLED

#ifdef WINDOWS_ENABLED
	HKEY hkey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Hardware\\Description\\System\\BIOS", 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {
		product.name = _read_registry_string(hkey, L"SystemProductName");
		product.family = _read_registry_string(hkey, L"SystemFamily");
		product.manufacturer = _read_registry_string(hkey, L"SystemManufacturer");
		product.board_name = _read_registry_string(hkey, L"BaseBoardProduct");
		RegCloseKey(hkey);
	}
#endif // WINDOWS_ENABLED

	return product;
}

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
	const String *value;
	if (value = fields.getptr("NAME"); value) {
		distro.name = *value;
	}
	if (value = fields.getptr("PRETTY_NAME"); value) {
		distro.pretty_name = *value;
	}
	if (value = fields.getptr("VERSION"); value) {
		distro.version = *value;
	} else if (value = fields.getptr("VERSION_ID"); value) {
		distro.version = *value;
	}
	if (value = fields.getptr("BUILD_ID"); value) {
		distro.build = *value;
	}
	if (value = fields.getptr("VARIANT_ID"); value) {
		distro.variant = *value;
	}
	// Update/release branch:
	// - SteamOS uses STEAMOS_DEFAULT_UPDATE_BRANCH
	// - Bazzite uses RELEASE_TYPE
	if (value = fields.getptr("STEAMOS_DEFAULT_UPDATE_BRANCH"); value) {
		distro.update_branch = *value;
	} else if (value = fields.getptr("RELEASE_TYPE"); value) {
		distro.update_branch = *value;
	}

	return distro;
}

bool _detect_steamos(const sentry::native::DistroInfo &p_distro) {
	if (p_distro.name.to_lower() == "steamos") {
		sentry::logging::print_debug("Detected SteamOS via os-release.");
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

bool _detect_steamdeck(const sentry::native::ProductInfo &p_product) {
	if (p_product.manufacturer == "Valve") {
		if (p_product.family == "Aerith" || p_product.family == "Sephiroth") {
			return true;
		}
	}
	return false;
}

} // unnamed namespace

namespace sentry::native {

const PlatformInfo &detect_platform() {
	static PlatformInfo cached_info;
	static bool first_run = true;

	if (first_run) {
		first_run = false;

		cached_info.product = _read_product_info();

#if defined(LINUX_ENABLED)
		cached_info.distro = _read_distro_info();
		cached_info.kernel_version = _read_kernel_version();
#elif defined(WINDOWS_ENABLED)
		cached_info.is_wine = _detect_wine();
		if (cached_info.is_wine) {
			cached_info.distro = _read_distro_info();
			cached_info.kernel_version = _read_kernel_version();
		}
#endif

		cached_info.is_steamos = _detect_steamos(cached_info.distro);
		cached_info.is_bazzite = _detect_bazzite(cached_info.distro);
		cached_info.is_steam = _detect_steam();
		cached_info.is_steamdeck = _detect_steamdeck(cached_info.product);
	}

	return cached_info;
}

} // namespace sentry::native
