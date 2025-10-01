"""
Tool to separate debug symbols.
"""

from SCons.Script import Builder, Dir, File, Clean, Exit, Action
import os
import os.path


def separate_debug_symbols(target, source, env):
    platform = env['platform']
    out_dir = env['out_dir']

    target_path = str(target[0])
    source_path = str(source[0])

    def run(cmd):
        err = os.system(cmd)
        return os.WEXITSTATUS(err)

    if platform in ["macos", "ios"]:
        err = run(f'dsymutil "{source_path}" -o "{target_path}"')
        if err != 0:
            print(f"ERROR: Failed to split debug symbols (exit code {err})")
            Exit(1)

        err = run(f'strip -u -r "{source_path}"')
        if err != 0:
            print(f"ERROR: Failed to strip debug symbols (exit code {err})")
            Exit(1)
    elif platform == "linux":
        debug_path = f"{source_path}.debug"

        err = run(f'objcopy --only-keep-debug --compress-debug-sections=zlib "{source_path}" "{target_path}"')
        if err != 0:
            print(f"ERROR: Failed to split debug symbols (exit code {err})")
            Exit(1)

        err = run(f'strip --strip-debug --strip-unneeded "{source_path}"')
        if err != 0:
            print(f"ERROR: Failed to strip debug symbols (exit code {err})")
            Exit(1)

        err = run(f'objcopy --add-gnu-debuglink="{target_path}" "{source_path}"')
        if err != 0:
            print(f"ERROR: Failed to add debug link (exit code {err})")
            Exit(1)
    else:
        print("ERROR: Can't separate debug symbols on this platform")
        Exit(1)


def command(env, target, source):
    result = env.Command(
        target,
        source,
        Action(separate_debug_symbols, cmdstr="Separating debug symbols: $SOURCE -> $TARGET")
    )
    Clean(target, target)
    return result


def generate(env):
    env.AddMethod(command, "SeparateDebugSymbols")


def exists(env):
    return True
