#!/usr/bin/env python
## Purpose: Build sentry-native static library.

import os


# *** Prologue: Initialization

try:
    Import("env")
    env = env.Clone()
except:
    env = Environment(ENV = os.environ)

# Get platform and architecture from environment or command line arguments
platform = env.get("platform", ARGUMENTS.get("platform", ""))
arch = env.get("arch", ARGUMENTS.get("arch", ""))

if not platform:
    print("ERROR: Platform not specified. Use platform=<platform> argument.")
    Exit(1)

if platform not in ["windows", "linux", "macos"]:
    Return("env")

if not arch:
    print("ERROR: Architecture not specified. Use arch=<arch> argument.")
    Exit(1)

if env.get("use_mingw") == True:
    print("ERROR: Compiling with MinGW is not supported yet.")
    Exit(1)

if env.get("use_llvm") == True:
    print("ERROR: Compiling with LLVM is not supported yet.")
    Exit(1)


# *** Chapter 1: Targets and variables

build_targets = []

def add_lib_target(lib_name):
    if platform == "windows":
        build_targets.append(File(f"sentry-native/install/lib/{lib_name}.lib"))
        build_targets.append(File(f"sentry-native/install/lib/{lib_name}.pdb"))
    else:
        build_targets.append(File(f"sentry-native/install/lib/lib{lib_name}.a"))
    env.Append(LIBS=[lib_name])

# Common libs
add_lib_target("sentry")
add_lib_target("crashpad_client")
add_lib_target("crashpad_handler_lib")
add_lib_target("crashpad_minidump")
add_lib_target("crashpad_snapshot")
add_lib_target("crashpad_tools")
add_lib_target("crashpad_util")
add_lib_target("crashpad_mpack")
add_lib_target("mini_chromium")

# Platform-specific libs
if platform == "windows":
    add_lib_target("crashpad_compat")
    env.Append(
        LIBS=[
            "winhttp",
            "advapi32",
            "DbgHelp",
            "Version",
        ]
    )
elif platform == "linux":
    add_lib_target("crashpad_compat")
    env.Append(
        LIBS=[
            "curl",
        ]
    )
elif platform == "macos":
    env.Append(
        LIBS=[
            "curl",
            "bsm"
        ],
        LINKFLAGS=[
            "-framework", "Foundation",
        ]
    )

# Crashpad handler
if platform == "windows":
    build_targets.append(File("sentry-native/install/bin/crashpad_handler.exe"))
    build_targets.append(File("sentry-native/install/bin/crashpad_wer.dll"))
    build_targets.append(File("sentry-native/install/bin/crashpad_wer.pdb"))
else:
    build_targets.append(File("sentry-native/install/bin/crashpad_handler"))

# Other defines
env.Append(CPPDEFINES=["SENTRY_BUILD_STATIC", "SDK_NATIVE"])
env.Append(CPPPATH=[Dir("sentry-native/include/")])
env.Append(LIBPATH=[Dir("sentry-native/install/lib/")])


# *** Chapter 2: CMake

cmake_gen = "cmake -B build"
cmake_gen += " -DSENTRY_BUILD_SHARED_LIBS=OFF"
cmake_gen += " -DSENTRY_BACKEND=crashpad"
cmake_gen += " -DSENTRY_SDK_NAME=\"sentry.native.godot\""
cmake_gen += " -DCMAKE_BUILD_TYPE=RelWithDebInfo"

if platform == "windows":
    cmake_gen += " -DSENTRY_BUILD_RUNTIMESTATIC=ON"
    if arch == "x86_32":
        cmake_arch = "-A Win32"
    elif arch == "x86_64":
        cmake_arch = "-A x64"
    else:
        print(f"ERROR: Unsupported architecture '{arch}' for platform '{platform}'")
        Exit(1)
    cmake_gen += f' -G "Visual Studio 17 2022" {cmake_arch}'
elif platform == "linux":
    if arch == "x86_32":
        cmake_gen += ' -DCMAKE_C_FLAGS="-m32" -DCMAKE_CXX_FLAGS="-m32" -DLINK_OPTIONS="-m32"'
    elif arch == "x86_64":
        cmake_gen += ' -DCMAKE_C_FLAGS="-m64" -DCMAKE_CXX_FLAGS="-m64" -DLINK_OPTIONS="-m64"'
    else:
        print(f"ERROR: Unsupported architecture '{arch}' for platform '{platform}'")
        Exit(1)
elif platform == "macos":
    cmake_arch = "arm64;x86_64" if arch == "universal" else arch
    cmake_gen += f' -DCMAKE_OSX_ARCHITECTURES="{cmake_arch}"'
    macos_deployment_target = env.get("macos_deployment_target", "default")
    if macos_deployment_target != "default":
        cmake_gen += f' -DCMAKE_OSX_DEPLOYMENT_TARGET=\"{macos_deployment_target}\"'

cmake_sentry = "cmake --build build --target sentry --parallel --config RelWithDebInfo"
cmake_crashpad = "cmake --build build --target crashpad_handler --parallel --config RelWithDebInfo"
cmake_install = "cmake --install build --prefix install --config RelWithDebInfo"


## *** Chapter 3: Build Commands

sentry_native_path = Dir('sentry-native').abspath

actions = [
        f"cd {sentry_native_path} && {cmake_gen}",
        f"cd {sentry_native_path} && {cmake_sentry}",
        f"cd {sentry_native_path} && {cmake_crashpad}",
        f"cd {sentry_native_path} && {cmake_install}",
    ]

# Copy crashpad handler PDB
if platform == "windows":
    target_file = File("sentry-native/install/bin/crashpad_handler.pdb")
    actions.append(
        Copy(
            target_file,
            File("sentry-native/build/crashpad_build/handler/RelWithDebInfo/crashpad_handler.pdb")
        )
    )
    build_targets.append(target_file)

# Use new environment importing current shell's env for greater compatibility.
env_cmake = Environment(ENV = os.environ)

sentry_native = env_cmake.Command(
    target=build_targets,
    source=[Dir("sentry-native/src")],
    action=actions
)

# Force sentry-native to be built sequential to godot-cpp (not in parallel)
Depends(sentry_native, "godot-cpp")

Default(sentry_native)

Clean(sentry_native, [Dir("sentry-native/build"), Dir("sentry-native/install")])


## *** Chapter 4: Export pseudo-builder to copy crashpad handler

def CopyCrashpadHandler(self, target_dir):
    actions = []
    targets = []
    sources = []

    def copy_file_action(target_file, source_file):
        actions.append(Copy(target_file, source_file))
        targets.append(target_file)
        sources.append(source_file)

    source_dir = Dir("modules/sentry-native/install/bin")

    if platform == "windows":
        copy_file_action(
            target_dir.File("crashpad_handler.exe"),
            source_dir.File("crashpad_handler.exe")
        )
        copy_file_action(
            target_dir.File("crashpad_handler.pdb"),
            source_dir.File("crashpad_handler.pdb")
        )
        copy_file_action(
            target_dir.File("crashpad_wer.dll"),
            source_dir.File("crashpad_wer.dll")
        )
        copy_file_action(
            target_dir.File("crashpad_wer.pdb"),
            source_dir.File("crashpad_wer.pdb")
        )
    else:
        copy_file_action(
            target_dir.File("crashpad_handler"),
            source_dir.File("crashpad_handler")
        )

    return env.Command(targets, sources, actions)

env.AddMethod(CopyCrashpadHandler, "CopyCrashpadHandler")


Return("env")
