# Sentry for Godot Engine

This SDK is actively evolving and may still be incomplete. It is developed as a C++ GDExtension library, building on top of existing Sentry SDKs, such as [sentry-native](https://github.com/getsentry/sentry-native). We are also considering adding support for compilation as a [custom module](https://docs.godotengine.org/en/stable/contributing/development/core_and_modules/custom_modules_in_cpp.html). [Let us know](https://github.com/getsentry/sentry-godot/discussions) what you think!

## Getting started

Pre-built extension libraries with the demo project are available in [**Releases**](https://github.com/getsentry/sentry-godot/releases).

Check the official [Sentry SDK documentation](https://docs.sentry.io/platforms/godot/) to get started.

In the Godot editor, you can adjust options by going to `Project Settings -> Sentry -> Options`. Feel free to explore the demo `project/` for usage examples.

## Building Sentry Godot SDK

Godot Sentry SDK can be built for Windows, Linux, and macOS platforms (x86_64 for PC, and universal `arch` for Mac).

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
    The build process should produce a GDExtension library file for the ***editor target*** at `project/addons/sentry/bin/...`.

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

## Contributing

We appreciate your contributions! Feel free to open issues for feature requests and ask questions in [**Discussions**](https://github.com/getsentry/sentry-godot/discussions). Your feedback is very much welcome!

Check out our [**Contributing Guide**](https://github.com/getsentry/sentry-godot/blob/master/CONTRIBUTING.md).
