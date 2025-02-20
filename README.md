# Sentry for Godot Engine

This SDK is actively evolving and may still be incomplete. Documentation is on the way, but in the meantime, feel free to explore the demo `project/` for usage examples.

## Building Sentry Godot SDK

Godot Sentry SDK can be built for Windows, Linux, and macOS platforms. Pre-built extension libraries with the demo project are available in [**Releases**](https://github.com/getsentry/sentry-godot/releases).

### Setting up SCons

Prerequisites: SCons, CMake, C/C++ compiler, python, clang-format.

On Windows, if you have `scoop` installed, you can easily install most of the required packages with the following command:
```
scoop install python scons cmake clang
```

You can also use an existing Python installation to install SCons build tool:
```bash
# install scons
python -m pip install scons

# upgrade scons
python -m pip install --upgrade scons
```
Or, on a Mac:

```
brew install scons
```

### Compiling

1. Clone this repository
2. Restore submodules: `git submodule update --init --recursive`
3. Build GDExtension libraries:
    ```bash
    # build *editor* library for the current platform
    # run from the repository root dir
    scons target=editor debug_symbols=yes
    ```
    The build process should produce a GDExtension library file for the ***editor target*** at `project/addons/sentrysdk/bin/...`.

    To export a project in Godot that uses this extension, you'll also need the libraries for the export templates:
    ```bash
    # build *export* library for the current platform
    scons target=template_release debug_symbols=yes
    ```
4. Open demo project in Godot Engine:
    ```bash
    # open demo project in Godot 4.3
    godot project/project.godot
    ```

In the Godot editor, you can adjust the Sentry SDK settings by going to `Project Settings -> Sentry -> Config`.
