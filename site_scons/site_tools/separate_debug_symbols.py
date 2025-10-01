"""
Tool to separate debug symbols.
"""

from SCons.Script import Builder, Copy, Dir, Clean
import os.path


def get_symbols_path(env, source):
    source_path = str(source[0])
    platform = env['platform']
    out_dir = env['out_dir']

    if platform in ["macos", "ios"]:
        source_name = os.path.basename(source_path)
        if source_name.endswith(".dylib"):
            source_name = os.path.splitext(source_name)[0]
        return Dir(f"{out_dir}/dSYMs/{source_name}.dSYM")
    elif platform == "linux":
        return File(f"{source_path}.debug")
    return ""


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


def command(env, source):
    symbols_path = get_symbols_path(env, source)
    result = env.Command(
        symbols_path,
        source,
        separate_debug_symbols,
        cmdstr = "Separate debug symbols"
    )
    Clean(symbols_path, symbols_path)
    return result


def generate(env):
    env.AddMethod(command, "SeparateDebugSymbols")


def exists(env):
    return True
