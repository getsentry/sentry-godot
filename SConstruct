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

# Include sentry-native libs (static)
if env["platform"] in ["linux", "windows", "macos"]:
    env.Append(CPPDEFINES=["SENTRY_BUILD_STATIC"])
    env.Append(CPPPATH=[env.Dir("sentry-native/include")])
    env.Append(LIBPATH=[
        "sentry-native/build-static/",
        "sentry-native/build-static/crashpad_build/client/",
        "sentry-native/build-static/crashpad_build/compat/",
        "sentry-native/build-static/crashpad_build/handler/",
        "sentry-native/build-static/crashpad_build/minidump/",
        "sentry-native/build-static/crashpad_build/snapshot/",
        "sentry-native/build-static/crashpad_build/third_party/mini_chromium/",
        "sentry-native/build-static/crashpad_build/tools/",
        "sentry-native/build-static/crashpad_build/util/",
        ])
    env.Append(LIBS=[
        "libsentry",
        "libcrashpad_client",
        "libcrashpad_compat",
        "libcrashpad_handler_lib",
        "libcrashpad_minidump",
        "libcrashpad_snapshot",
        "libcrashpad_tools",
        "libcrashpad_util",
        "libcurl",
        "libmini_chromium",
        ])

# Build libsentry.
# TODO: macOS needs to use a different SDK.
if env["platform"] in ["linux", "windows", "macos"]:
    libsentry = env.Command(
        "sentry-native/build-static/libsentry.a",
        [],
        [
            "cd sentry-native; cmake -B build-static -D SENTRY_BUILD_SHARED_LIBS=OFF -D CMAKE_BUILD_TYPE=RelWithDebInfo -D SENTRY_BACKEND=crashpad -D SENTRY_SDK_NAME=sentry.native.godot",
            "cd sentry-native; cmake --build build-static --target sentry --parallel",
        ]
    )
    Default(libsentry)

# Build crashpad handler.
if env["platform"] == "linux":
    crashpad_handler = env.Command(
        "project/addons/sentrysdk/bin/crashpad_handler",
        [],
        [
            "cd sentry-native; cmake --build build-static --target crashpad_handler --config Release --parallel",
            Copy("project/addons/sentrysdk/bin/crashpad_handler",
                "sentry-native/build-static/crashpad_build/handler/crashpad_handler"),
        ]
    )
    Default(crashpad_handler)
elif env["platform"] == "windows":
    crashpad_handler = env.Command(
        "project/addons/sentrysdk/bin/crashpad_handler.exe",
        [],
        [
            "cd sentry-native; cmake --build build-static --target crashpad_handler --config Release --parallel",
            Copy("project/addons/sentrysdk/bin/crashpad_handler.exe",
                "sentry-native/build-static/crashpad_build/handler/crashpad_handler.exe"),
        ]
    )
    Default(crashpad_handler)

# Source files to compile.
sources = Glob("src/*.cpp")
sources += Glob("src/runtime/*.cpp")
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
