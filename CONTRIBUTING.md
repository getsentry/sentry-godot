# Contributing Guide

## Building SDK

Sentry SDK for Godot Engine can be built for Windows x86_64, Linux x86_64, and macOS universal platforms. Support for more platforms and architectures are expected to be added in time.

### Prerequisites

- C/C++ compiler
- SCons build tool and Python
- CMake -- to build sentry-native SDK
- clang-format & pre-commit -- for style checks

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

## Project Structure

- `src/` -- Godot extension source code
- `modules/` -- various submodules, such as `godot-cpp` and other SDKs like `sentry-native`
- `project/` -- example Godot project
- `project/addons/sentrysdk/` -- where build artifacts are placed
- `project/test/` -- unit tests
- `scripts/` -- various scripts used mostly for maintenance
- `doc_classes/` -- built-in Godot documentation (class reference)

## Formatting Code

Please run `clang-format` before submitting a PR to adhere to our code style. You can also install [pre-commit](https://pre-commit.com/) hooks for automatic formatting on commit:
```sh
pre-commit install
```

## Documentation

### Sentry documentation

We maintain official documentation in https://github.com/getsentry/sentry-docs. This is our main documentation. New features and changes should be reflected in that repository in a linked PR. See [Contributing to Docs](https://docs.sentry.io/contributing/).

### Built-in documentation

We also maintain a built-in Godot class reference. It is available offline right inside Godot editor. Each of the classes we export begins with "Sentry", so it's easy to find our API using the **Search Help** dialog. This documentation is located in the `doc_classes/` directory, with each class stored in a corresponding XML file. The structure of these files is auto-generated, while the documentation text itself is added manually.

If you add or modify the public API, you must first regenerate these files and then update their content accordingly. To regenerate the XML documentation, run the following command from the `project/` directory:

```sh
godot --doctool ../ --gdextension-docs
```

This command generates XML files for new classes and updates existing ones by adding or removing class members based on your changes. Ensure that you **compile the library** with `target=editor` for your current platform before running this command; otherwise, your changes will not be detected!

> 🛈 You can run `scripts/update-doc-classes.ps1` which wraps the same operation.

Once the XML files are regenerated, you can begin updating the documentation for the affected classes. It is recommended to run regeneration process again after completing your changes, as this will correct certain style issues automatically.

## Testing

> 🛈 Our CI automatically runs tests for open PRs.

Testing is performed using the **gdUnit4** testing framework in GDScript. Unit tests (and other types of tests) are located in the `project/test/` directory. These tests can be executed from the Godot editor, except for isolated tests (see below).

Some tests require isolation, meaning they need specific options to be set and must be executed in a separate process. These tests are located in the `project/test/isolated/` directory. We have a PowerShell script for running such tests in bulk: `scripts/run-isolated-tests.ps1`.
