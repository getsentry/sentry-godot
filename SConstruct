#!/usr/bin/env python
import os
import sys
import subprocess

# Global Setting.
GODOT_CPP_REF = "godot-4.3-stable"
PROJECT_DIR = "project"
EXTENSION_NAME = "sentrysdk"
COMPATIBILITY_MINIMUM = "4.3"

BIN_DIR = "{project_dir}/addons/{extension_name}/bin".format(
    project_dir=PROJECT_DIR,
    extension_name=EXTENSION_NAME)

# Checkout godot-cpp...
if not os.path.exists("godot-cpp"):
    print("Cloning godot-cpp repository...")
    result = subprocess.run(
        ["git", "clone", "-b", GODOT_CPP_REF, "https://github.com/godotengine/godot-cpp.git"],
        check=True,
    )
    if result.returncode != 0:
        print("Error: Cloning godot-cpp repository failed.")
        Exit(1)
    print("Finished cloning godot-cpp repository.")

env = SConscript("godot-cpp/SConstruct")

# Build sentry-native.
# TODO: macOS needs to use a different SDK.
if env["platform"] in ["linux", "macos"]:

    def build_sentry_native(target, source, env):
        result = subprocess.run(
            ["sh", "scripts/build-sentry-native.sh"],
            check=True,
        )
        return result.returncode

    sentry_native = env.Command(
        ["sentry-native/install/lib/libsentry.a", BIN_DIR + "/crashpad_handler"],
        ["sentry-native/src"],
        [
            build_sentry_native,
            Copy(
                BIN_DIR + "/crashpad_handler",
                "sentry-native/install/bin/crashpad_handler",
            ),
        ],
    )
elif env["platform"] == "windows":

    def build_sentry_native(target, source, env):
        result = subprocess.run(
            ["powershell", "scripts/build-sentry-native.ps1"],
            check=True,
        )
        return result.returncode

    sentry_native = env.Command(
        ["sentry-native/install/lib/sentry.lib", BIN_DIR + "/crashpad_handler.exe"],
        ["sentry-native/src/"],
        [
            build_sentry_native,
            Copy(
                BIN_DIR + "/crashpad_handler.exe",
                "sentry-native/install/bin/crashpad_handler.exe",
            ),
        ],
    )
Depends(sentry_native, "godot-cpp")  # Force sentry-native to be built sequential to godot-cpp (not in parallel)
Default(sentry_native)
Clean(sentry_native, ["sentry-native/build", "sentry-native/install"])

# Include sentry-native libs (static).
if env["platform"] in ["linux", "macos", "windows"]:
    env.Append(CPPDEFINES=["SENTRY_BUILD_STATIC"])
    env.Append(CPPPATH=["sentry-native/include"])
    env.Append(LIBPATH=["sentry-native/install/lib/"])
    env.Append(
        LIBS=[
            "sentry",
            "crashpad_client",
            "crashpad_compat",
            "crashpad_handler_lib",
            "crashpad_minidump",
            "crashpad_snapshot",
            "crashpad_tools",
            "crashpad_util",
            "mini_chromium",
        ]
    )
# Include additional platform-specific libs.
if env["platform"] == "windows":
    env.Append(
        LIBS=[
            "winhttp",
            "advapi32",
            "DbgHelp",
            "Version",
        ]
    )
elif env["platform"] == "linux":
    env.Append(
        LIBS=[
            "curl",
        ]
    )

# Source files to compile.
sources = Glob("src/*.cpp")
# To add subdirectories to compilation: sources += Glob("src/some_dir/*.cpp")

# Build library.
if env["platform"] == "macos":
    library = env.SharedLibrary(
        "{bin_dir}/lib{name}.{platform}.{target}.framework/liblimboai.{platform}.{target}".format(
            bin_dir=BIN_DIR,
            name=EXTENSION_NAME,
            platform=env["platform"],
            target=env["target"],
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "{bin_dir}/lib{name}{suffix}{shlib_suffix}".format(
            bin_dir=BIN_DIR,
            name=EXTENSION_NAME,
            suffix=env["suffix"],
            shlib_suffix=env["SHLIBSUFFIX"],
        ),
        source=sources,
    )

Default(library)

# Deploy extension manifest.
manifest = env.Substfile(
    target="{bin_dir}/{name}.gdextension".format(
        bin_dir=BIN_DIR,
        name=EXTENSION_NAME,
    ),
    source="src/manifest.gdextension",
    SUBST_DICT={
        "{name}": EXTENSION_NAME,
        "{compatibility_minimum}": COMPATIBILITY_MINIMUM,
    },
)

Default(manifest)
