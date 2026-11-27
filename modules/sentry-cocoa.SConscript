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
platform = env["platform"]
if platform == "macos":
    sdk = "macosx"
else:
    sdk = "iphonesimulator" if env["ios_simulator"] else "iphoneos"

cocoa_build_dir = project_root / "build/sentry-cocoa" / sdk
archive = cocoa_build_dir / "lib/SentryObjC" / sdk / "libSentryObjC-Debug.a"
headers = cocoa_build_dir / "include/SentryObjC"


def build_cocoa(target, source, env):
    if not (cocoa_source / "Package.swift").is_file():
        print("ERROR: Initialize Cocoa with git submodule update --init modules/sentry-cocoa")
        return 1

    if not shutil.which("xcbeautify"):
        print("ERROR: Cocoa builds require xcbeautify. Install it with brew install xcbeautify")
        return 1

    # Keep each SDK's DerivedData beside its archive until dsymutil has linked the extension.
    subprocess.run([
        "bash", "scripts/build-xcframework-sentryobjc.sh", "--variant", "static",
        "--sdks", sdk, "--output-dir", str(cocoa_build_dir),
    ], cwd=cocoa_source, check=True)
    remove_if_exists(headers)
    shutil.copytree(cocoa_source / "Sources/SentryObjC/Public", headers)


cocoa_inputs = [File("sentry-cocoa.SConscript")]
if (cocoa_source / "Package.swift").is_file():
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=cocoa_source)
    changes = subprocess.check_output(["git", "diff", "HEAD", "--binary"], cwd=cocoa_source)
    cocoa_inputs.extend([Value(revision), Value(hashlib.sha256(changes).hexdigest())])
xcode = subprocess.check_output(["xcodebuild", "-version"])
cocoa_build = env.Command(
    [File(archive), File(headers / "SentryObjC.h")],
    cocoa_inputs + [Value(xcode)],
    build_cocoa,
)
env.Precious(cocoa_build)
env.NoCache(cocoa_build)
env.Clean(cocoa_build, Dir(cocoa_build_dir))
env.Alias("sentry_cocoa", cocoa_build)
env["cocoa_build"] = cocoa_build

swiftc = Path(subprocess.check_output(["xcrun", "--find", "swiftc"], text=True).strip())
env.Append(
    CPPPATH=[str(headers.parent)],
    CCFLAGS=["-fobjc-arc"],
    LIBPATH=[str(swiftc.parent.parent / "lib/swift" / sdk), "/usr/lib/swift"],
    LIBS=["z", "c++"],
    FRAMEWORKS=["Foundation", "CoreData", "CoreGraphics", "QuartzCore"],
    LINKFLAGS=["-Xlinker", "-force_load", "-Xlinker", str(archive), "-Wl,-rpath,/usr/lib/swift"],
)
for framework in ("SystemConfiguration", "AVFoundation", "CoreMedia", "CoreVideo", "MetricKit",
                  "PDFKit", "SwiftUI", "WebKit", "AppKit" if platform == "macos" else "UIKit"):
    env.Append(LINKFLAGS=["-weak_framework", framework])


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


env.AddMethod(CreateXCFrameworkFromLibs, "CreateXCFrameworkFromLibs")
Return("env")
