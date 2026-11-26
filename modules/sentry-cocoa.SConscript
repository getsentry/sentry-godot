#!/usr/bin/env python
## Purpose: macOS and iOS-specific build configuration using sentry-cocoa

import hashlib
import subprocess
import shutil
from pathlib import Path

Import("env")


def remove_if_exists(filesystem_path):
    """Remove directory tree, file or link."""
    path = Path(filesystem_path)
    if path.is_symlink():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(path)
    elif path.is_file():
        path.unlink()


project_root = Path(env.Dir("#").abspath)
cocoa_source = project_root / "modules/sentry-cocoa"
cocoa_build_dir = project_root / "build/sentry-cocoa"
xcframework_path = cocoa_build_dir / "SentryObjC-Dynamic.xcframework"
macos_headers = cocoa_build_dir / "macos_include/SentryObjC"
slices = ("macos-arm64_x86_64", "ios-arm64", "ios-arm64_x86_64-simulator")
build_outputs = [
    File(xcframework_path / "macos-arm64_x86_64/SentryObjC.framework/Versions/A/SentryObjC"),
    Dir(macos_headers),
    *[Dir(xcframework_path / name / "dSYMs/SentryObjC.framework.dSYM") for name in slices],
    *[Dir(xcframework_path / name) for name in slices[1:]],
]


def build_cocoa(target, source, env):
    if not (cocoa_source / "Sentry.xcodeproj").exists():
        print("ERROR: Initialize Cocoa with git submodule update --init modules/sentry-cocoa")
        return 1

    fingerprint = hashlib.sha256()
    for command in (
        ["git", "rev-parse", "HEAD"],
        ["git", "diff", "HEAD", "--binary"],
        ["xcodebuild", "-version"],
    ):
        fingerprint.update(subprocess.check_output(command, cwd=cocoa_source))
    fingerprint.update((project_root / "modules/sentry-cocoa.SConscript").read_bytes())
    build_id = fingerprint.hexdigest()
    stamp = cocoa_build_dir / ".build-id"
    if stamp.exists() and stamp.read_text() == build_id and all(Path(str(node)).exists() for node in target):
        print("Sentry Cocoa build is up to date.")
        return 0

    if not shutil.which("xcbeautify"):
        print("ERROR: Cocoa builds require xcbeautify. Install it with brew install xcbeautify")
        return 1

    remove_if_exists(cocoa_build_dir)
    subprocess.run([
        "bash", "scripts/build-xcframework-sentryobjc.sh",
        "--variant", "dynamic", "--sdks", "macosx,iphoneos,iphonesimulator",
    ], cwd=cocoa_source, check=True)
    cocoa_build_dir.mkdir(parents=True, exist_ok=True)
    shutil.move(str(cocoa_source / xcframework_path.name), xcframework_path)

    # macOS compiles against SentryObjC headers while linking the deployed libSentry.dylib.
    macos_headers.mkdir(parents=True)
    framework = xcframework_path / "macos-arm64_x86_64/SentryObjC.framework"
    for directory in ("Headers", "PrivateHeaders"):
        for header in (framework / directory).glob("*.h"):
            shutil.copy2(header, macos_headers)
    if not all(Path(str(node)).exists() for node in target):
        print("ERROR: Cocoa build is missing required framework slices or debug symbols.")
        return 1
    stamp.write_text(build_id)
    return 0


cocoa_build = env.Command(build_outputs, File("sentry-cocoa.SConscript"), build_cocoa)
# The action checks its fingerprint so restored CI artifacts work without SCons' signature database.
env.AlwaysBuild(cocoa_build)
env.Precious(cocoa_build)
env.NoCache(cocoa_build)
env.Clean(cocoa_build, Dir(cocoa_build_dir))
env.Alias("sentry_cocoa", cocoa_build)
env["cocoa_build"] = cocoa_build


platform = env["platform"]
ios_simulator = env.get("ios_simulator", False)


# Configuring Cocoa framework.
if platform in ["macos", "ios"]:
    project_root = Path(env.Dir("#").abspath)

    env.Append(
        LINKFLAGS=[
            "-framework", "Foundation",
        ],
        # Enable ARC for Objective-C files
        CCFLAGS=["-fobjc-arc"]
    )

    # Add Sentry Cocoa framework to compilation.

    if platform == "macos":
        framework_dir = xcframework_path / "macos-arm64_x86_64/SentryObjC.framework"
        env.Append(
            CPPPATH=str(macos_headers.parent),
            LIBPATH=f"{project_root}/project/addons/sentry/bin/macos/",
            LIBS=["Sentry"],
            LINKFLAGS=[
                # Load libSentry.dylib from same dir as GDExtension.
                "-Wl,-rpath,@loader_path/"
            ]
        )
    else:
        if ios_simulator:
            framework_dir = xcframework_path / "ios-arm64_x86_64-simulator/SentryObjC.framework"
        else:
            framework_dir = xcframework_path / "ios-arm64/SentryObjC.framework"

        framework_container_dir = framework_dir.parent.absolute()

        env.Append(
            CPPFLAGS=["-F" + str(framework_container_dir)],
            LINKFLAGS=[
                "-framework", "SentryObjC",
                "-F" + str(framework_container_dir),
                # Allow extension to find framework in addons/sentry/ directory.
                "-Wl,-rpath,@loader_path/..",
            ]
        )

        print(f"Added {platform} Sentry dynamic framework: {framework_dir}")


# *** Export pseudo-builders to create xcframeworks

def CreateXCFrameworkFromLibs(self, framework_path, libraries):
    """Create an xcframework from dynamic libraries.

    Each dylib is wrapped in a .framework bundle before packaging into the
    xcframework. This produces proper framework slices that don't require
    additional conversion by Godot's export pipeline.
    """
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

        # Use a common framework name across all slices, derived from
        # the xcframework name (e.g. "libsentry.ios.debug").
        fw_name = Path(framework_path).stem

        temp_dirs = []
        temp_frameworks = []
        for lib in libraries:
            lib_path = Path(lib)
            # Each slice needs its own directory since all frameworks share
            # the same name.
            slice_dir = lib_path.parent / lib_path.stem
            fw_dir = slice_dir / f"{fw_name}.framework"

            remove_if_exists(slice_dir)
            fw_dir.mkdir(parents=True, exist_ok=True)

            binary_dest = fw_dir / fw_name
            shutil.copy2(lib_path, binary_dest)

            # Dylib's LC_ID_DYLIB still points to the bare .dylib path; update
            # it to the framework layout so dyld resolves it correctly at runtime.
            install_name = f"@rpath/{fw_name}.framework/{fw_name}"
            result = subprocess.run(
                ["install_name_tool", "-id", install_name, str(binary_dest)],
                capture_output=True, text=True,
            )
            if result.returncode != 0:
                print(f"ERROR: install_name_tool failed: {result.stderr}")
                return 1

            is_simulator = "simulator" in lib_path.name
            env.WriteFrameworkPlist(
                fw_dir / "Info.plist",
                bundle_executable=fw_name,
                bundle_identifier=f"io.sentry.godot.{fw_name}",
                bundle_platforms=["iPhoneSimulator"] if is_simulator else ["iPhoneOS"],
                min_os_version=env.get("ios_min_version", "15.0"),
            )

            temp_dirs.append(slice_dir)
            temp_frameworks.append(fw_dir)

        cmd = ["xcodebuild", "-create-xcframework"]
        for fw_dir in temp_frameworks:
            cmd.extend(["-framework", str(fw_dir)])
        cmd.extend(["-output", framework_path])

        print(f"Running: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True)

        for temp_dir in temp_dirs:
            remove_if_exists(temp_dir)

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

        # The prebuilt addon needs neither headers nor module maps at runtime.
        for framework in Path(target_path).glob("*/*.framework"):
            for directory in ("Headers", "PrivateHeaders", "Modules"):
                remove_if_exists(framework / directory)

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
    source_xcframework = xcframework_path
    target_dir_path = Path(str(target_dir))

    commands = []

    def copy_debug_symbols(target, source):
        copied = env.Copy(target, source)
        env.AddPreAction(copied, Delete("$TARGET"))
        return copied

    if platform == "ios":
        slice_dirs = [
            source_xcframework / "ios-arm64",
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
        commands.append(
            copy_debug_symbols(
                Dir(target_dir_path / "dSYMs" / "SentryObjC-ios-arm64.framework.dSYM"),
                Dir(source_xcframework / "ios-arm64" / "dSYMs" / "SentryObjC.framework.dSYM"),
            )
        )

        commands.append(
            copy_debug_symbols(
                Dir(target_dir_path / "dSYMs" / "SentryObjC-ios-arm64_x86_64-simulator.framework.dSYM"),
                Dir(source_xcframework / "ios-arm64_x86_64-simulator" / "dSYMs" / "SentryObjC.framework.dSYM")
            )
        )

    elif platform == "macos":
        source_framework = source_xcframework / "macos-arm64_x86_64/SentryObjC.framework"

        def patch_install_name_action(target, source, env):
            """Patch install name after copy"""
            lib_path_str = str(target[0])
            print(f"  Patching install name for binary: {lib_name}")
            try:
                new_rpath = "@rpath/libSentry.dylib"
                cmd = ["install_name_tool", "-id", new_rpath, lib_path_str]
                subprocess.run(cmd, capture_output=True, text=True, check=True)
                print(f"  Successfully updated install name to: {new_rpath}")
            except subprocess.CalledProcessError as e:
                print(f"  WARNING: Failed to update install name: {e}")
                print(f"  stdout: {e.stdout}")
                print(f"  stderr: {e.stderr}")
                return 1
            return 0

        # Copy the framework binary as dylib, and patch install name.
        lib_name = "libSentry.dylib"
        lib_path = target_dir_path / lib_name
        commands.append(
            env.Command(
                File(lib_path),
                File(source_framework / "Versions/A/SentryObjC"),
                [
                    Copy("$TARGET", "$SOURCE"),
                    patch_install_name_action
                ]
            )
        )

        # Debug symbols
        commands.append(
            copy_debug_symbols(
                Dir(target_dir_path / "dSYMs" / "libSentry.dylib.dSYM"),
                Dir(source_xcframework / "macos-arm64_x86_64" / "dSYMs" / "SentryObjC.framework.dSYM")
            )
        )

    else:
        print("ERROR: Unexpected platform: ", platform)
        Exit(1)

    env.Depends(commands, cocoa_build)
    return commands


# Export pseudo-builders
env.AddMethod(CreateXCFrameworkFromLibs, "CreateXCFrameworkFromLibs")
env.AddMethod(CreateXCFrameworkFromSlices, "CreateXCFrameworkFromSlices")
env.AddMethod(DeploySentryCocoa, "DeploySentryCocoa")

Return("env")
