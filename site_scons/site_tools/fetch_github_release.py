"""
Tool to fetch a zip asset from a GitHub release.

Downloads a zip asset from a GitHub release, extracts it to a
target directory, and caches by version to avoid re-downloading.

Usage in SConstruct:
    env.Tool("fetch_github_release")
    env.FetchGithubRelease(
        "modules/my-package.properties",
        asset="my-package.zip",
        target_dir="target/directory/",
        strip_prefix="some/prefix/directory/",
    )
"""

import shutil
import urllib.request
import zipfile
from pathlib import Path
from SCons.Script import Exit
from utils import read_property


def fetch_github_release(env, properties_file, asset, target_dir, strip_prefix="", clean=False, version_file=".version"):
    """Download and extract a zip asset from a GitHub release."""
    project_root = Path(env.Dir("#").abspath)
    properties_path = project_root / properties_file

    if not properties_path.exists():
        print(f"ERROR: Properties file not found at {properties_path}")
        Exit(1)

    repo = read_property("repo", properties_path)
    version = read_property("version", properties_path)

    target_path = project_root / target_dir
    version_file_path = target_path / version_file

    # Check if we need to download.
    should_download = True
    if version_file_path.exists():
        stored_version = version_file_path.read_text().strip()
        if stored_version == version:
            should_download = False
            print(f"Detected {asset} v{version} -- up-to-date!")

    if should_download:
        print(f"Fetching {asset} v{version}")

        # Clean existing directory to remove stale files from previous versions.
        if clean and target_path.exists():
            if not target_path.resolve().is_relative_to(project_root.resolve()):
                print(f"ERROR: Refusing to clean directory outside project root: {target_path}")
                Exit(1)
            shutil.rmtree(target_path)

        url = f"{repo}/releases/download/{version}/{asset}"
        zip_path = target_path / asset

        target_path.mkdir(parents=True, exist_ok=True)

        try:
            print(f"Downloading {url}")
            urllib.request.urlretrieve(url, zip_path)

            print(f"Extracting {zip_path}")
            with zipfile.ZipFile(zip_path, "r") as z:
                prefix = strip_prefix.strip("/") + "/" if strip_prefix else ""
                for info in z.infolist():
                    if not info.filename.startswith(prefix):
                        continue
                    # Strip the prefix from the path.
                    rel_path = info.filename[len(prefix):]
                    if not rel_path:
                        continue
                    out_path = target_path / rel_path
                    if info.is_dir():
                        out_path.mkdir(parents=True, exist_ok=True)
                    else:
                        out_path.parent.mkdir(parents=True, exist_ok=True)
                        with z.open(info) as src, open(out_path, "wb") as dst:
                            dst.write(src.read())

            zip_path.unlink()
            version_file_path.write_text(version)

            print(f"Successfully fetched {asset} v{version}.")

        except Exception as e:
            print(f"ERROR: Failed to download {asset}: {e}")
            Exit(1)


def command(env, properties_file, asset, target_dir, strip_prefix="", clean=False, version_file=".version"):
    target = str(Path(target_dir) / version_file)

    def action(target, source, env):
        fetch_github_release(env, properties_file, asset, target_dir, strip_prefix, clean, version_file)

    result = env.Command(target, properties_file, action)
    return result


def generate(env):
    env.AddMethod(command, "FetchGithubRelease")


def exists(env):
    return True
