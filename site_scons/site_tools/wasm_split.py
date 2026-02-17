"""
Tool that provides wasm-split binary from Sentry symbolicator repo.

Downloads the wasm-split binary from symbolicator GitHub releases and
exposes it via env["WASM_SPLIT"] for use by other tools.
"""

import urllib.request
import platform
import stat
from pathlib import Path
from SCons.Script import Exit
from utils import read_property


def download_wasm_split(env, properties_file):
    """Download wasm-split binary if needed, return path to binary."""
    if not properties_file.exists():
        print(f"ERROR: Properties file not found at {properties_file}")
        Exit(1)

    repo = read_property("repo", properties_file)
    version = read_property("version", properties_file)

    wasm_split_dir = properties_file.parent / "wasm-split"
    version_file = wasm_split_dir / ".version"

    # Determine platform-specific binary name.
    system = platform.system()
    machine = platform.machine()
    if system == "Linux" and machine in ("x86_64", "aarch64"):
        binary_name = f"wasm-split-Linux-{machine}"
    elif system == "Darwin":
        binary_name = "wasm-split-Darwin-universal"
    elif system == "Windows" and machine in ("x86_64", "AMD64"):
        binary_name = "wasm-split-Windows-x86_64.exe"
    else:
        print(f"ERROR: wasm-split is not available for {system} {machine}")
        Exit(1)

    binary_path = wasm_split_dir / binary_name

    # Check if we need to download.
    should_download = True
    if version_file.exists() and binary_path.exists():
        stored_version = version_file.read_text().strip()
        if stored_version == version:
            should_download = False
            print(f"Detected wasm-split v{version} -- up-to-date!")

    if should_download:
        print(f"Fetching wasm-split v{version}")

        wasm_split_dir.mkdir(parents=True, exist_ok=True)

        url = f"{repo}/releases/download/{version}/{binary_name}"
        print(f"Downloading {url}")

        try:
            urllib.request.urlretrieve(url, binary_path)
        except Exception as e:
            print(f"ERROR: Failed to download wasm-split: {e}")
            Exit(1)

        # Make binary executable.
        binary_path.chmod(binary_path.stat().st_mode | stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH)

        version_file.write_text(version)
        print(f"Successfully fetched wasm-split v{version}.")

    return str(binary_path)


def generate(env, **kwargs):
    properties_file = Path(env.Dir("#").abspath) / kwargs.get("properties_file", "")
    env["WASM_SPLIT"] = download_wasm_split(env, properties_file)


def exists(env):
    return True
