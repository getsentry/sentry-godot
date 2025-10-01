"""
Tool to create copy commands with auto-clean.
"""

from SCons.Script import Builder, Copy, Dir, Clean
import os.path


def copy_command(env, target, source):
    result = env.Command(
        target,
        source,
        Copy("$TARGET", "$SOURCE")
    )
    Clean(target, target)
    return result


def generate(env):
    env.AddMethod(copy_command, "Copy")


def exists(env):
    return True
