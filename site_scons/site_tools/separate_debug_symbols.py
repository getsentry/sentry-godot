"""
Tool to separate debug symbols.

Adds SeparateDebugSymbols pseudo-builder that registers a post-action for splitting
symbols, with a cleanup.
"""

from SCons.Script import Clean, Exit, Action
import os

def separate_debug_symbols(target, source, env, p_symbols_path):
    platform = env["platform"]

    binary_path = str(target[0])
    symbols_path = str(p_symbols_path)

    if platform in ["macos", "ios"]:
        err = env.Execute(f'dsymutil "{binary_path}" -o "{symbols_path}"')
        if err != 0:
            print(f"ERROR: Failed to split debug symbols (exit code {err})")
            Exit(1)

        err = env.Execute(f'strip -u -r "{binary_path}"')
        if err != 0:
            print(f"ERROR: Failed to strip debug symbols (exit code {err})")
            Exit(1)

    elif platform == "linux":
        err = env.Execute(f'objcopy --only-keep-debug --compress-debug-sections=zlib "{binary_path}" "{symbols_path}"')
        if err != 0:
            print(f"ERROR: Failed to split debug symbols (exit code {err})")
            Exit(1)

        err = env.Execute(f'strip --strip-debug --strip-unneeded "{binary_path}"')
        if err != 0:
            print(f"ERROR: Failed to strip debug symbols (exit code {err})")
            Exit(1)

        err = env.Execute(f'objcopy --add-gnu-debuglink="{symbols_path}" "{binary_path}"')
        if err != 0:
            print(f"ERROR: Failed to add debug link (exit code {err})")
            Exit(1)

    elif platform == "android":
        err = env.Execute(f'llvm-objcopy --only-keep-debug --compress-debug-sections=zlib "{binary_path}" "{symbols_path}"')
        if err != 0:
            print(f"ERROR: Failed to split debug symbols (exit code {err})")
            Exit(1)

        err = env.Execute(f'llvm-strip --strip-debug --strip-unneeded "{binary_path}"')
        if err != 0:
            print(f"ERROR: Failed to strip debug symbols (exit code {err})")
            Exit(1)

        err = env.Execute(f'llvm-objcopy --add-gnu-debuglink="{symbols_path}" "{binary_path}"')
        if err != 0:
            print(f"ERROR: Failed to add debug link (exit code {err})")
            Exit(1)

    elif platform == "web":
        wasm_split = env["WASM_SPLIT"]
        err = env.Execute(f'"{wasm_split}" "{binary_path}" --strip --debug-out="{symbols_path}"')
        if err != 0:
            print(f"ERROR: Failed to split WASM debug symbols (exit code {err})")
            Exit(1)

    else:
        print("ERROR: Can't separate debug symbols on this platform")
        Exit(1)


def command(env, symbols_path, binary):
    # Closure captures symbols_path
    def action(target, source, env):
        separate_debug_symbols(target, source, env, symbols_path)

    env.AddPostAction(binary, Action(action, cmdstr=f"Separating debug symbols: {binary} -> {symbols_path}"))
    env.Clean(binary, symbols_path)
    return binary


def generate(env):
    env.AddMethod(command, "SeparateDebugSymbols")


def exists(env):
    return True
