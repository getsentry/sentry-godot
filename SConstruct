#!/usr/bin/env python
import os
import sys
import subprocess

# Global Setting.
GODOT_CPP_REF = "godot-4.3-stable"
PROJECT_DIR = "project"
EXTENSION_NAME = "sentrysdk"
COMPATIBILITY_MINIMUM = "4.3"

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
        ["sentry-native/install/lib/libsentry.a", "project/addons/sentrysdk/bin/crashpad_handler"],
        ["sentry-native/src"],
        [
            build_sentry_native,
            Copy("project/addons/sentrysdk/bin/crashpad_handler",
                "sentry-native/install/bin/crashpad_handler"),
        ]
    )
elif env["platform"] == "windows":

    def build_sentry_native(target, source, env):
        result = subprocess.run(
            ["powershell", "scripts/build-sentry-native.ps1"],
            check=True,
        )
        return result.returncode

    sentry_native = env.Command(
        ["sentry-native/install/lib/sentry.lib", "project/addons/sentrysdk/bin/crashpad_handler.exe"],
        ["sentry-native/src/"],
        [
            build_sentry_native,
            Copy("project/addons/sentrysdk/bin/crashpad_handler.exe",
                "sentry-native/install/bin/crashpad_handler.exe"),
        ]
    )
Depends(sentry_native, "godot-cpp") # Force sentry-native to be built sequential to godot-cpp (not in parallel)
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
        ])
# Include additional libs on Windows that are needed for sentry-native.
if env["platform"] == "windows":
    env.Append(
        LIBS=[
            "winhttp",
            "advapi32",
            "DbgHelp",
            "Version",
        ])

# Source files to compile.
sources = Glob("src/*.cpp")
# sources += Glob("some_dir/*.cpp")

# Build library.
if env["platform"] == "macos":
    library = env.SharedLibrary(
        "{project_dir}/addons/{name}/bin/lib{name}.{platform}.{target}.framework/liblimboai.{platform}.{target}".format(
            project_dir=PROJECT_DIR,
            name=EXTENSION_NAME,
            platform=env["platform"],
            target=env["target"],
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "{project_dir}/addons/{name}/bin/lib{name}{suffix}{shlib_suffix}".format(
            project_dir=PROJECT_DIR,
            name=EXTENSION_NAME,
            suffix=env["suffix"],
            shlib_suffix=env["SHLIBSUFFIX"],
        ),
        source=sources,
    )

Default(library)

# Deploy extension manifest.
manifest = env.Substfile(
    target="{project_dir}/addons/{name}/bin/{name}.gdextension".format(
        project_dir=PROJECT_DIR,
        name=EXTENSION_NAME,
    ),
    source="src/manifest.gdextension",
    SUBST_DICT={
        "{name}": EXTENSION_NAME,
        "{compatibility_minimum}": COMPATIBILITY_MINIMUM,
    },
)

Default(manifest)
