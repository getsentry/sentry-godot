#pragma once

#include <godot_cpp/variant/string.hpp>

using namespace godot;

namespace sentry::native {

struct WineProtonInfo {
	bool is_wine = false;
	bool is_proton = false;
	String version; // "10.0-4", "10.0-20260113", "9-27"
	String runtime_name; // "Wine", "Proton", "Proton Experimental", "GE-Proton"
};

struct DistroInfo {
	String name; // "SteamOS", "Bazzite"
	String pretty_name; // "Ubuntu 24.04.3 LTS"
	String version; // "3.7.17", "43.20260303.0 (Kinoite)", "24.04.4 LTS (Noble Numbat)"
	String build; // "20251027.1", "Stable (F43.20260303)"
	String variant; // "steamdeck", "bazzite-deck"
	String update_branch; // "stable", "beta"
};

// SMBIOS system and baseboard identity
struct ProductInfo {
	String name; // "Inspiron 15 5510", "Jupiter"
	String manufacturer; // "Dell Inc.", "Valve"
	String family; // "Inspiron", "Aerith"
	String board_name; // "0MJFHK", "Jupiter"
};

struct PlatformInfo {
	ProductInfo product;
	WineProtonInfo wine_proton;
	DistroInfo distro;
	String kernel_version;
	bool is_steamos = false;
	bool is_bazzite = false;
	bool is_steam = false;
	bool is_steamdeck = false;
};

// Runs platform detection checks. Results are cached after the first call.
const PlatformInfo &detect_platform();

} // namespace sentry::native
