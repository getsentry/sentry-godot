#!/usr/bin/env python
## Purpose: Build sentry-native libraries and crash handlers.

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

shared = env.get("sentry_native_shared", False)
windows = platform == "windows"
root = Dir("sentry-native").abspath
backend_option = env.get("sentry_native_backend", "both" if shared else "native")
backends = ["native", "crashpad"] if backend_option == "both" else [backend_option]
default_backend = backends[0]


def backend_dirs(backend):
    if not shared and backend == "native":
        return "build", "install"
    linkage = "shared" if shared else "static"
    variant = f"{platform}-{arch}/{backend}"
    return f"build-{linkage}/{variant}", f"install-{linkage}/{variant}"


_, install_dir = backend_dirs(default_backend)
library_name = "sentry.dll" if windows else "libsentry.so"
library_dir = "bin" if windows else "lib"
executable_suffix = ".exe" if windows else ""
artifacts = {}

# Common libs
static_libs = ["sentry"]
if not shared and default_backend == "crashpad":
    static_libs += [
        "crashpad_client",
        "crashpad_handler_lib",
        "crashpad_minidump",
        "crashpad_snapshot",
        "crashpad_tools",
        "crashpad_util",
        "crashpad_mpack",
        "mini_chromium",
        "crashpad_compat",
    ]
env.Append(LIBS=static_libs)

# Platform-specific libs
if windows:
    # Platform detection uses the registry independently of sentry-native linkage.
    env.Append(LIBS=["advapi32"])

if not shared:
    if windows:
        env.Append(LIBS=[
            "winhttp",
            "DbgHelp",
            "Version",
            "synchronization",
        ])
    elif platform == "linux":
        static_libs.append("unwind")
        env.Append(LIBS=[
            "unwind",
            "curl",
        ])

# Other defines
env.Append(CPPDEFINES=["SDK_NATIVE", "SENTRY_NATIVE_SHARED" if shared else "SENTRY_BUILD_STATIC"])
env.Append(CPPDEFINES=[f"SENTRY_HANDLER_{backend.upper()}" for backend in backends])
env.Append(CPPPATH=[Dir("sentry-native/include")])
env.Append(LIBPATH=[Dir(f"sentry-native/{install_dir}/lib")])


# *** Chapter 2: CMake

cmake_flags = [
    "-DSENTRY_BUILD_SHARED_LIBS=" + ("ON" if shared else "OFF"),
    "-DSENTRY_SDK_NAME=sentry.native.godot",
    "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
    "-DCMAKE_INSTALL_LIBDIR=lib",
    "-DSENTRY_BUILD_TESTS=OFF",
    "-DSENTRY_BUILD_EXAMPLES=OFF",
]

if platform in ["windows", "linux"] and arch not in ["x86_32", "x86_64", "arm64"]:
    print(f"ERROR: Unsupported architecture '{arch}' for platform '{platform}'")
    Exit(1)

if windows:
    cmake_arch = {"x86_32": "Win32", "x86_64": "x64", "arm64": "ARM64"}[arch]
    cmake_flags += ['-G "Visual Studio 17 2022"', f"-A {cmake_arch}", "-DSENTRY_BUILD_RUNTIMESTATIC=ON"]
elif platform == "linux":
    if arch in ["x86_32", "x86_64"]:
        bits = "32" if arch == "x86_32" else "64"
        cmake_flags += [f"-DCMAKE_C_FLAGS=-m{bits}", f"-DCMAKE_CXX_FLAGS=-m{bits}"]
        if arch == "x86_32":
            cmake_flags.append("-DSENTRY_BUILD_FORCE32=ON")
    elif arch == "arm64":
        pass  # Native compilation on arm64 host; no special flags needed.


## *** Chapter 3: Build Commands

# Use new environment importing current shell's env for greater compatibility.
env_cmake = Environment(ENV=os.environ)
previous_build = None

for backend in backends:
    build, install = backend_dirs(backend)
    handler = "sentry-crash" if backend == "native" else "crashpad_handler"
    handler_name = handler + executable_suffix

    # Native crash handler
    files = {handler_name: f"{install}/bin/{handler_name}"}
    if shared:
        files[library_name] = f"{install}/{library_dir}/{library_name}"
    if windows:
        wer = "sentry-wer" if backend == "native" else "crashpad_wer"
        files[f"{wer}.dll"] = f"{install}/bin/{wer}.dll"
        if shared or backend == "crashpad":
            pdb_dir = f"{build}/RelWithDebInfo" if backend == "native" else f"{build}/crashpad_build/handler/RelWithDebInfo"
            if shared:
                files["sentry.pdb"] = f"{install}/bin/sentry.pdb"
            files[f"{handler}.pdb"] = f"{pdb_dir}/{handler}.pdb"
            files[f"{wer}.pdb"] = f"{pdb_dir}/{wer}.pdb"
        else:
            files[f"{handler}.pdb"] = f"{install}/bin/{handler}.pdb"
            files[f"{wer}.pdb"] = f"{install}/bin/{wer}.pdb"

    targets = [File(f"sentry-native/{path}") for path in files.values()]
    if shared:
        if windows:
            targets.append(File(f"sentry-native/{install}/lib/sentry.lib"))
    else:
        for lib_name in static_libs:
            if windows:
                targets += [File(f"sentry-native/{install}/lib/{lib_name}.{suffix}") for suffix in ["lib", "pdb"]]
            else:
                targets.append(File(f"sentry-native/{install}/lib/lib{lib_name}.a"))

    flags = cmake_flags + [f"-DSENTRY_BACKEND={backend}"]
    build_target = " --target sentry" if not shared and backend == "native" else ""
    actions = [
        f'cd "{root}" && cmake -B {build} ' + " ".join(flags),
        f'cd "{root}" && cmake --build {build}{build_target} --config RelWithDebInfo --parallel',
        f'cd "{root}" && cmake --install {build} --prefix {install} --config RelWithDebInfo',
    ]

    # Copy native crash handler PDB
    if windows and not shared and backend == "native":
        actions.append(Copy(
            File(f"sentry-native/{install}/bin/{handler}.pdb"),
            File(f"sentry-native/{build}/RelWithDebInfo/{handler}.pdb"),
        ))

    nodes = env_cmake.Command(targets, [Dir("sentry-native/src"), File("sentry-native/CMakeLists.txt")], actions)
    # Force sentry-native to be built sequential to godot-cpp (not in parallel)
    Depends(nodes, "godot-cpp")
    if previous_build is not None:
        Depends(nodes, previous_build)
    previous_build = nodes
    Default(nodes)
    Clean(nodes, [Dir(f"sentry-native/{build}"), Dir(f"sentry-native/{install}")])
    artifacts[backend] = files


## *** Chapter 4: Export pseudo-builder to copy native crash handler

def CopyCrashHandler(self, target_dir):
    deployed = []
    for backend, files in artifacts.items():
        destinations = []
        if shared:
            destinations.append(target_dir.Dir(backend))
        if backend == default_backend:
            # The editor uses the default backend; exports package the selected variant.
            destinations.append(target_dir)
        for name, path in files.items():
            source = File(f"modules/sentry-native/{path}")
            for destination in destinations:
                node = self.Command(destination.File(name), source, Copy("$TARGET", "$SOURCE"))
                deployed += node
                if platform == "linux" and self["separate_debug_symbols"]:
                    self.SeparateDebugSymbols(str(node[0]) + ".debug", node)
    return deployed


env.AddMethod(CopyCrashHandler, "CopyCrashHandler")
Return("env")
