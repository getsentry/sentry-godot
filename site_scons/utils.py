"""
Reusable utility and action functions
"""

import os

def separate_debug_symbols(target, source, env):
    target_path = str(target[0])
    platform = env['platform']
    out_dir = env['out_dir']

    def run(cmd):
        err = os.system(cmd)
        return os.WEXITSTATUS(err)

    if platform in ["macos", "ios"]:
        target_name = os.path.basename(target_path)
        if target_name.endswith(".dylib"):
            target_name = os.path.splitext(target_name)[0]
        dsym_path = f"{out_dir}/dSYMs/{target_name}.dSYM"

        err = run(f'dsymutil "{target_path}" -o "{dsym_path}"')
        if err != 0:
            print(f"ERROR: Failed to split debug symbols (exit code {err})")
            Exit(1)

        err = run(f'strip -u -r "{target_path}"')
        if err != 0:
            print(f"ERROR: Failed to strip debug symbols (exit code {err})")
            Exit(1)
    elif platform == "linux":
        debug_path = f"{target_path}.debug"

        err = run(f'objcopy --only-keep-debug --compress-debug-sections=zlib "{target_path}" "{debug_path}"')
        if err != 0:
            print(f"ERROR: Failed to split debug symbols (exit code {err})")
            Exit(1)

        err = run(f'strip --strip-debug --strip-unneeded "{target_path}"')
        if err != 0:
            print(f"ERROR: Failed to strip debug symbols (exit code {err})")
            Exit(1)

        err = run(f'objcopy --add-gnu-debuglink="{debug_path}" "{target_path}"')
        if err != 0:
            print(f"ERROR: Failed to add debug link (exit code {err})")
            Exit(1)
