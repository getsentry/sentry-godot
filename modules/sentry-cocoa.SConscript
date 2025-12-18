#!/usr/bin/env python
## Purpose: macOS and iOS-specific build configuration using sentry-cocoa

import urllib.request
import zipfile
import os
import stat
import subprocess
import shutil
from pathlib import Path

Import("env")


def get_property(prop_name, file_path):
    """Read property from .properties file"""
    with open(file_path, 'r') as file:
        for line in file:
            if line.startswith(prop_name):
                return line.split('=')[1].strip().strip('"')
    raise KeyError(f"Property '{prop_name}' not found in {file_path}")


def remove_if_exists(filesystem_path):
    """Remove directory tree, file or link."""
    path = Path(filesystem_path)
    if path.is_dir():
        shutil.rmtree(path)
    elif path.is_file() or path.is_symlink():
        path.unlink()


def extract_zip_with_symlinks(zip_path, extract_to):
    """Extract zip file preserving symlinks

    Python's zipfile.extractall() doesn't preserve symlinks - it extracts them
    as regular files containing the symlink target path. This function detects
    symlinks by their external_attr and creates proper symlinks instead.
    """
    with zipfile.ZipFile(zip_path, 'r') as zip_archive:
        for info in zip_archive.infolist():
            # NOTE: external_attr contains Unix permissions in upper 16 bits.
            unix_mode = info.external_attr >> 16
            if stat.S_ISLNK(unix_mode):
                # Symlink – create manually.
                target_path = zip_archive.read(info.filename).decode('utf-8')
                source_path = Path(extract_to) / info.filename

                parent_dir = source_path.parent
                if parent_dir != Path('.'):
                    parent_dir.mkdir(parents=True, exist_ok=True)

                try:
                    os.symlink(target_path, str(source_path))
                except OSError as e:
                    print(f"ERROR: Failed to create symlink {source_path} -> {target_path}: {e}")
                    Exit(1)
            else:
                # Regular file or directory – extract normally.
                zip_archive.extract(info, extract_to)


def detect_xcode():
    """Get the current Xcode path using xcode-select"""
    try:
        result = subprocess.run(['xcode-select', '--print-path'],
                              capture_output=True, text=True, check=True)
        xcode_path = result.stdout.strip()
        print(f"Detected Xcode path: {xcode_path}")
        return xcode_path
    except Exception as e:
        print(f"ERROR: Failed to get Xcode path: {e}")
        Exit(1)


def flatten_framework(framework_path):
    """Flatten macOS framework by removing symlinks and versioned structure"""
    framework_path = Path(framework_path)
    if not framework_path.exists():
        print(f"ERROR: Framework not found at {framework_path}")
        Exit(1)

    versions_dir = framework_path / "Versions"
    if not versions_dir.exists():
        # Framework doesn't need flattening.
        return

    print(f"Flattening framework structure: {framework_path}")

    # 1. Remove all symlinks from the root of the framework
    for item_path in framework_path.iterdir():
        if item_path.is_symlink():
            print(f"  Removing symlink: {item_path.name}")
            item_path.unlink()

    # 2. Move Versions/A/* to root of the framework
    version_a_dir = versions_dir / "A"
    if version_a_dir.exists():
        print("  Moving contents from Versions/A/ to root")
        for item_path in version_a_dir.iterdir():
            dest_path = framework_path / item_path.name

            # Handle potential conflicts
            if dest_path.exists():
                print(f"  WARNING: {item_path.name} already exists at root, removing old version")
                remove_if_exists(dest_path)

            shutil.move(str(item_path), str(dest_path))
            print(f"    Moved: {item_path.name}")
    else:
        print(f"  WARNING: Versions/A directory not found in {framework_path}")

    # 3. Remove Versions/ directory
    print("  Removing Versions directory")
    shutil.rmtree(versions_dir)

    # 4. Patch the binary's install name for flattened structure
    binary_name = framework_path.name.replace('.framework', '')
    binary_path = framework_path / binary_name

    if not binary_path.exists():
        print(f"ERROR: Framework binary not found at {binary_path}")
        Exit(1)

    print(f"  Patching install name for binary: {binary_name}")
    install_name = f"@rpath/{binary_name}.framework/{binary_name}"
    cmd = ["install_name_tool", "-id", install_name, str(binary_path)]

    try:
        subprocess.run(cmd, capture_output=True, text=True, check=True)
        print(f"  Successfully updated install name to: {install_name}")
    except subprocess.CalledProcessError as e:
        print(f"  WARNING: Failed to update install name: {e}")
        print(f"  stdout: {e.stdout}")
        print(f"  stderr: {e.stderr}")

    print(f"Framework flattening completed: {framework_path}")


def update_cocoa_framework():
    """Updates Sentry Cocoa to the latest version."""
    project_root = Path(env.Dir("#").abspath)

    properties_file = project_root / "modules/sentry-cocoa.properties"
    if not properties_file.exists():
        print(f"ERROR: Properties file not found at {properties_file}")
        Exit(1)

    cocoa_repo = get_property("repo", properties_file)
    cocoa_version = get_property("version", properties_file)

    cocoa_dir = project_root / "modules/sentry-cocoa"
    version_file = cocoa_dir / ".version"

    # Check if we need to download
    should_download = True
    if version_file.exists():
        try:
            stored_version = version_file.read_text().strip()
            if stored_version == cocoa_version:
                # Check if framework actually exists
                xcframework_path = cocoa_dir / "Sentry.xcframework"
                if xcframework_path.exists():
                    should_download = False
                    print(f"Detected Sentry Cocoa SDK v{cocoa_version} – up-to-date!")
        except:
            pass

    if should_download:
        print(f"Fetching Sentry Cocoa SDK v{cocoa_version}")

        remove_if_exists(cocoa_dir)

        cocoa_dir.mkdir(parents=True, exist_ok=True)

        # Download dynamic framework
        framework_url = f"{cocoa_repo}/releases/download/{cocoa_version}/Sentry.xcframework.zip"
        zip_path = cocoa_dir / "Sentry.xcframework.zip"

        try:
            print(f"Downloading {framework_url}")
            urllib.request.urlretrieve(framework_url, zip_path)

            print(f"Extracting {zip_path}")
            extract_zip_with_symlinks(zip_path, cocoa_dir)

            # NOTE: We need to flatten macOS slice due to issues with symlinks on Windows.
            # flatten_framework(cocoa_dir / "Sentry.xcframework" / "macos-arm64_x86_64" / "Sentry.framework")

            zip_path.unlink() # delete file
            version_file.write_text(cocoa_version)

            print(f"Successfully fetched Sentry.xcframework {cocoa_version}.")

        except Exception as e:
            print(f"ERROR: Failed to download Sentry framework: {e}.")
            Exit(1)


update_cocoa_framework()

platform = env["platform"]
ios_simulator = env.get("ios_simulator", False)


# Configuring Cocoa framework.
if platform in ["macos", "ios"]:
    project_root = Path(env.Dir("#").abspath)
    xcframework_path = project_root / "modules/sentry-cocoa/Sentry.xcframework"

    if not xcframework_path.exists():
        print(f"ERROR: Sentry.xcframework is missing at {xcframework_path}.")
        Exit(1)

    env.Append(
        LINKFLAGS=[
            "-framework", "Foundation",
        ],
        # Enable ARC for Objective-C files
        CCFLAGS=["-fobjc-arc"]
    )

    # Add Sentry Cocoa framework to compilation.

    if platform == "macos":
        framework_dir = xcframework_path / "macos-arm64_arm64e_x86_64/Sentry.framework"
        env.Append(
            LINKFLAGS=[
                # Swift libraries
                f"-L{detect_xcode()}/Toolchains/XcodeDefault.xctoolchain/usr/lib/swift/macosx"
            ]
        )
    else:
        if ios_simulator:
            framework_dir = xcframework_path / "ios-arm64_x86_64-simulator/Sentry.framework"
            env.Append(
                LINKFLAGS=[
                    # Swift libraries
                    f"-L{detect_xcode()}/Toolchains/XcodeDefault.xctoolchain/usr/lib/swift/iphonesimulator"
                ]
            )
        else:
            framework_dir = xcframework_path / "ios-arm64_arm64e/Sentry.framework"
            env.Append(
                LINKFLAGS=[
                    # Swift libraries
                    f"-L{detect_xcode()}/Toolchains/XcodeDefault.xctoolchain/usr/lib/swift/iphoneos"
                ]
            )


    if not framework_dir.exists():
        print(f"ERROR: Sentry.framework is missing at {framework_dir}.")
        Exit(1)

    framework_container_dir = framework_dir.parent.absolute()

    env.Append(
        CPPFLAGS=["-F" + str(framework_container_dir)],
        LINKFLAGS=[
            "-framework", "Sentry",
            "-F" + str(framework_container_dir),
            # Allow extension to find framework in addons/sentry/ directory.
            "-Wl,-rpath,@loader_path/..",
        ]
    )

    print(f"Added {platform} Sentry dynamic framework: {framework_dir}")


# *** Export pseudo-builders to create xcframeworks

def CreateXCFrameworkFromLibs(self, framework_path, libraries):
    """Create an xcframework from multiple libraries."""
    framework_path = str(framework_path)

    def create_xcframework_action(target, source, env):
        # Check if all expected libraries exist at build time
        missing_libs = [lib for lib in libraries if not Path(lib).exists()]

        if missing_libs:
            print("ERROR: Missing libraries for xcframework:")
            for lib in missing_libs:
                print(f"  {lib}")
            print("Tip: Build all required variants first.")
            return 1

        print(f"Creating xcframework from {len(libraries)} libraries...")

        remove_if_exists(framework_path)

        # Generate xcodebuild command
        cmd = ["xcodebuild", "-create-xcframework"]
        for lib in libraries:
            cmd.extend(["-library", str(lib)])
        cmd.extend(["-output", framework_path])

        print(f"Running: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            print("ERROR: Failed creating xcframework:")
            print(result.stdout)
            print(result.stderr)
            return 1

        print(f"Successfully created: {framework_path}")
        return 0

    xcframework = env.Command(
        framework_path,
        libraries,
        create_xcframework_action
    )
    env.AlwaysBuild(xcframework)
    Clean(xcframework, framework_path)

    return xcframework


def CreateXCFrameworkFromSlices(self, target_path, slice_dirs):
    """Create an xcframework from selected framework slices."""
    target_path = str(target_path)

    def create_framework_from_slices_action(target, source, env):
        # Check if all expected slice directories exist at build time
        missing_slices = [slice_dir for slice_dir in slice_dirs if not Path(slice_dir).exists()]

        if missing_slices:
            print("ERROR: Missing framework slices:")
            for slice_dir in missing_slices:
                print(f"  {slice_dir}")
            return 1

        print(f"Creating xcframework from {len(slice_dirs)} framework slices...")

        remove_if_exists(target_path)

        # Run xcodebuild command
        cmd = ["xcodebuild", "-create-xcframework"]
        for slice_dir in slice_dirs:
            # Each slice directory should contain a .framework
            slice_path = Path(slice_dir)
            framework_path = None
            if slice_path.is_dir():
                for item in slice_path.iterdir():
                    if item.name.endswith('.framework'):
                        framework_path = item
                        break

            if not framework_path or not framework_path.exists():
                print(f"ERROR: No .framework found in slice directory: {slice_dir}")
                return 1

            cmd.extend(["-framework", str(framework_path)])

        cmd.extend(["-output", target_path])

        print(f"Running: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            print("ERROR: Failed creating xcframework from slices:")
            print(result.stdout)
            print(result.stderr)
            return 1

        print(f"Successfully created xcframework: {target_path}")
        return 0

    xcframework = env.Command(
        Dir(target_path),
        slice_dirs,
        create_framework_from_slices_action
    )
    env.AlwaysBuild(xcframework)
    Clean(xcframework, Dir(target_path))

    return xcframework


def DeploySentryCocoa(self, target_dir):
    """Deploy Sentry Cocoa framework to target directory."""

    platform = env["platform"]
    project_root = Path(env.Dir("#").abspath)
    source_xcframework = project_root / "modules/sentry-cocoa/Sentry.xcframework"
    target_dir_path = Path(str(target_dir))

    commands = []

    if platform == "ios":
        slice_dirs = [
            source_xcframework / "ios-arm64_arm64e",
            source_xcframework / "ios-arm64_x86_64-simulator"
        ]
        target_framework = target_dir_path / "Sentry.xcframework"

        commands.append(
            env.CreateXCFrameworkFromSlices(
                target_path=target_framework,
                slice_dirs=slice_dirs
            )
        )
        Clean(commands, Dir(target_framework))

        # Debug symbols
        # commands.append(
        #     env.Copy(
        #         Dir(target_dir_path / "dSYMs" / "Sentry-ios-arm64.framework.dSYM"),
        #         Dir(source_xcframework / "ios-arm64" / "dSYMs" / "Sentry.framework.dSYM"),
        #     )
        # )

        # commands.append(
        #     env.Copy(
        #         Dir(target_dir_path / "dSYMs" / "Sentry-ios-arm64_x86_64-simulator.framework.dSYM"),
        #         Dir(source_xcframework / "ios-arm64_x86_64-simulator" / "dSYMs" / "Sentry.framework.dSYM")
        #     )
        # )

    elif platform == "macos":
        source_framework = source_xcframework / "macos-arm64_arm64e_x86_64/Sentry.framework"
        target_framework = target_dir_path / "Sentry.framework"

        # Copy only the binary and "Resources" dir -- we don't need to export headers or modules.
        commands.append(
            env.Copy(File(target_framework / "Sentry"), File(source_framework / "Sentry"))
        )
        commands.append(
            env.Copy(Dir(target_framework / "Resources"), Dir(source_framework / "Resources"))
        )

        # Debug symbols
        # commands.append(
        #     env.Copy(
        #         Dir(target_dir_path / "dSYMs" / "Sentry.framework.dSYM"),
        #         Dir(source_xcframework / "macos-arm64_arm64e_x86_64" / "dSYMs" / "Sentry.framework.dSYM")
        #     )
        # )

    else:
        print("ERROR: Unexpected platform: ", platform)
        Exit(1)

    return commands


# Export pseudo-builders
env.AddMethod(CreateXCFrameworkFromLibs, "CreateXCFrameworkFromLibs")
env.AddMethod(CreateXCFrameworkFromSlices, "CreateXCFrameworkFromSlices")
env.AddMethod(DeploySentryCocoa, "DeploySentryCocoa")

Return("env")
