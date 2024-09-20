#!/usr/bin/env python
import os
import sys
import subprocess

# Global Setting.
GODOT_CPP_REF = "godot-4.3-stable"
PROJECT_DIR = "project"
EXTENSION_NAME = "myextension"
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
