#pragma once

#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::native {

struct WineProtonInfo {
	bool is_wine = false;
	bool is_proton = false;
	String wine_version; // "9.0", "8.0-3"
	String proton_version; // "Proton 8.0", "Proton Experimental"

	String get_version() const {
		return is_proton && !proton_version.is_empty() ? proton_version : wine_version;
	}
};

struct DistroInfo {
	String name; // "SteamOS", "Bazzite"
	String pretty_name; // "Ubuntu 24.04.3 LTS"
	String version; // "3.7.17", "43.20260303.0 (Kinoite)"
	String codename; // "holo", "Kinoite"
	String build; // "20251027.1", "Stable (F43.20260303)"
	String variant; // "steamdeck", "bazzite-deck"
	String image_id; // "bazzite-deck-43.20260303"
	String update_branch; // "stable", "beta"
};

struct PlatformInfo {
	WineProtonInfo wine_proton;
	DistroInfo distro;
	String kernel_version;
	bool is_steamos = false;
	bool is_bazzite = false;
	bool is_steam = false;
};

// Runs platform detection checks. Results are cached after the first call.
const PlatformInfo &detect_platform();

} // namespace sentry::native
