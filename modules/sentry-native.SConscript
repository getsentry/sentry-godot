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

backend = env.get("crash_backend", ARGUMENTS.get("crash_backend", "crashpad"))
if backend not in ("crashpad", "native"):
    print("ERROR: crash_backend must be crashpad or native")
    Exit(1)
variant = f"{platform}/{arch}/{env.get('target', 'editor')}/{backend}"
build_dir = f"build/{variant}"
install_dir = f"install/{variant}"
handler = "crashpad_handler" if backend == "crashpad" else "sentry-crash"
wer = "crashpad_wer" if backend == "crashpad" else "sentry-wer"
build_targets = []

def add_lib_target(lib_name):
    if platform == "windows":
        build_targets.append(File(f"sentry-native/{install_dir}/lib/{lib_name}.lib"))
        build_targets.append(File(f"sentry-native/{install_dir}/lib/{lib_name}.pdb"))
    else:
        build_targets.append(File(f"sentry-native/{install_dir}/lib/lib{lib_name}.a"))
    env.Append(LIBS=[lib_name])

# Common libs
add_lib_target("sentry")
if backend == "crashpad":
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
    if backend == "crashpad":
        add_lib_target("crashpad_compat")
    env.Append(
        LIBS=[
            "winhttp",
            "advapi32",
            "DbgHelp",
            "Version",
            "synchronization",
        ]
    )
elif platform == "linux":
    if backend == "crashpad":
        add_lib_target("crashpad_compat")
    add_lib_target("unwind")
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

# Crash handler
if platform == "windows":
    for filename in (f"{handler}.exe", f"{handler}.pdb", f"{wer}.dll", f"{wer}.pdb"):
        build_targets.append(File(f"sentry-native/{install_dir}/bin/{filename}"))
else:
    build_targets.append(File(f"sentry-native/{install_dir}/bin/{handler}"))

# Other defines
env.Append(CPPDEFINES=["SENTRY_BUILD_STATIC", "SDK_NATIVE", f"SENTRY_CRASH_BACKEND_{backend.upper()}"])
env.Append(CPPPATH=[Dir("sentry-native/include/")])
env.Append(LIBPATH=[Dir(f"sentry-native/{install_dir}/lib/")])


# *** Chapter 2: CMake

cmake_gen = f"cmake -B {build_dir}"
cmake_gen += " -DSENTRY_BUILD_SHARED_LIBS=OFF"
cmake_gen += f" -DSENTRY_BACKEND={backend}"
cmake_gen += " -DCMAKE_INSTALL_LIBDIR=lib"
cmake_gen += " -DSENTRY_SDK_NAME=\"sentry.native.godot\""
cmake_gen += " -DCMAKE_BUILD_TYPE=RelWithDebInfo"

if platform == "windows":
    cmake_gen += " -DSENTRY_BUILD_RUNTIMESTATIC=ON"
    if arch == "x86_32":
        cmake_arch = "-A Win32"
    elif arch == "x86_64":
        cmake_arch = "-A x64"
    elif arch == "arm64":
        cmake_arch = "-A ARM64"
    else:
        print(f"ERROR: Unsupported architecture '{arch}' for platform '{platform}'")
        Exit(1)
    cmake_gen += f' -G "Visual Studio 17 2022" {cmake_arch}'
elif platform == "linux":
    if arch == "x86_32":
        cmake_gen += ' -DSENTRY_BUILD_FORCE32=ON -DCMAKE_C_FLAGS="-m32" -DCMAKE_CXX_FLAGS="-m32" -DLINK_OPTIONS="-m32"'
    elif arch == "x86_64":
        cmake_gen += ' -DCMAKE_C_FLAGS="-m64" -DCMAKE_CXX_FLAGS="-m64" -DLINK_OPTIONS="-m64"'
    elif arch == "arm64":
        pass  # Native compilation on arm64 host; no special flags needed.
    else:
        print(f"ERROR: Unsupported architecture '{arch}' for platform '{platform}'")
        Exit(1)
elif platform == "macos":
    cmake_arch = "arm64;x86_64" if arch == "universal" else arch
    cmake_gen += f' -DCMAKE_OSX_ARCHITECTURES="{cmake_arch}"'
    macos_deployment_target = env.get("macos_deployment_target", "default")
    if macos_deployment_target != "default":
        cmake_gen += f' -DCMAKE_OSX_DEPLOYMENT_TARGET=\"{macos_deployment_target}\"'

cmake_sentry = f"cmake --build {build_dir} --target sentry --parallel {GetOption('num_jobs')} --config RelWithDebInfo"
cmake_handler = f"cmake --build {build_dir} --target {handler} --parallel {GetOption('num_jobs')} --config RelWithDebInfo"
cmake_install = f"cmake --install {build_dir} --prefix {install_dir} --config RelWithDebInfo"


## *** Chapter 3: Build Commands

sentry_native_path = Dir('sentry-native').abspath

actions = [
        f'cd "{sentry_native_path}" && {cmake_gen}',
        f'cd "{sentry_native_path}" && {cmake_sentry}',
        f'cd "{sentry_native_path}" && {cmake_handler}',
        f'cd "{sentry_native_path}" && {cmake_install}',
    ]

# Copy crash handler PDB.
if platform == "windows" and backend == "native":
    actions.append(Copy(
        File(f"sentry-native/{install_dir}/bin/{handler}.pdb"),
        File(f"sentry-native/{build_dir}/RelWithDebInfo/{handler}.pdb")
    ))

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

Clean(sentry_native, [Dir(f"sentry-native/{build_dir}"), Dir(f"sentry-native/{install_dir}")])


## *** Chapter 4: Export pseudo-builder to copy crash handler

def CopyCrashHandler(self, target_dir):
    actions = []
    targets = []
    sources = []

    def copy_file_action(target_file, source_file):
        actions.append(Copy(target_file, source_file))
        targets.append(target_file)
        sources.append(source_file)

    source_dir = Dir(f"#modules/sentry-native/{install_dir}/bin")
    filenames = [handler]
    if platform == "windows":
        filenames = [f"{handler}.exe", f"{handler}.pdb", f"{wer}.dll", f"{wer}.pdb"]
    for filename in filenames:
        copy_file_action(target_dir.File(filename), source_dir.File(filename))

    return env.Command(targets, sources, actions)

env.AddMethod(CopyCrashHandler, "CopyCrashHandler")


Return("env")
