"""
Tool to create framework symlinks as post-process action.

Usage: env.LinkBundle(FRAMEWORK_PATH, FRAMEWORK_TARGET)

This tool expects the framework path to contain a Versions/A subdirectory
with complete bundle resources after the specified targets are built.

Example:
    env.LinkBundle("mylib.framework/", [framework_resources])
"""

import os
from SCons.Script import Action


def link_bundle_action(target, source, env, framework_path):
    """Post-process action to create framework symlinks"""
    framework_dir = os.path.abspath(framework_path)
    version_dir = f"{framework_dir}/Versions/A"

    if not os.path.exists(version_dir):
        print(f"Versions/A directory does not exist: {version_dir}")
        return False

    original_cwd = os.getcwd()
    os.chdir(framework_dir)

    try:
        # Create Current -> A symlink in Versions directory
        versions_dir = os.path.dirname(version_dir)
        current_link = os.path.join(versions_dir, "Current")
        version_name = os.path.basename(version_dir)

        if os.path.islink(current_link):
            os.unlink(current_link)

        os.chdir(versions_dir)
        os.symlink(version_name, "Current")
        print(f"Created symlink: Current -> {version_name}")

        os.chdir(framework_dir)

        # Create symlinks in framework root that point to Versions/Current/*
        for item in os.listdir(version_dir):
            link_name = item
            relative_target = os.path.join("Versions", "Current", item)

            if os.path.islink(link_name):
                os.unlink(link_name)

            os.symlink(relative_target, link_name)
            print(f"Created symlink: {link_name} -> {relative_target}")
    finally:
        os.chdir(original_cwd)

    return True


def command(env, framework_path, target):
    # Closure captures framework_path
    def action(target, source, env):
        link_bundle_action(target, source, env, framework_path)

    env.AddPostAction(target, Action(action, cmdstr=f"Creating framework symlinks in {framework_path}"))
    return target


def generate(env):
    env.AddMethod(command, "LinkBundle")


def exists(env):
    return True
