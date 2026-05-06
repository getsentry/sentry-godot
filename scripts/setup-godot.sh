#!/usr/bin/env bash
#
# Download a Godot Engine binary (see usage() below).

set -euo pipefail

# --- Parse arguments ---

usage() {
    cat <<'EOF' >&2
Usage: setup-godot.sh [--version <ver>] [--platform <name>] [--arch <arch>] [--mono] [--dest <dir>]

Download a Godot engine binary into <dest>. When running under GitHub
Actions, GODOT and GODOT_VERSION are appended to $GITHUB_ENV and echoed
to stdout.

Options:
  --version <ver>     Godot version (default: 4.5.1-stable)
  --platform <name>   linux | macos | windows
                      (auto-detected from RUNNER_OS or uname when omitted)
  --arch <arch>       x86_32 | x86_64 | arm64 | universal
                      (auto-detected from RUNNER_ARCH or uname when omitted)
  --mono              Download the .NET (mono) build of the engine
  --dest <dir>        Directory to extract into (default: ./godot)

Environment variables:
  RUNNER_OS         Linux | macOS | Windows (used by --platform auto-detect when unset, falls back to uname)
  RUNNER_ARCH       X86 | X64 | ARM64 (used by --arch auto-detect when unset, falls back to uname -m)
  GITHUB_ENV        When set, GODOT and GODOT_VERSION are appended

Requirements: curl, unzip
EOF
    exit "$1"
}

VERSION="4.5.1-stable"
PLATFORM=""
ARCH=""
MONO=false
DEST="$PWD/godot"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)  VERSION="$2"; shift 2;;
        --platform) PLATFORM="$2"; shift 2;;
        --arch)     ARCH="$2"; shift 2;;
        --mono)     MONO=true; shift;;
        --dest)     DEST="$2"; shift 2;;
        -h|--help)  usage 0;;
        *) echo "Unknown option: $1" >&2; usage 1;;
    esac
done

# --- Resolve platform and arch (fall back to RUNNER_*/uname when not specified) ---

if [[ -z "$PLATFORM" ]]; then
    if [[ -n "${RUNNER_OS:-}" ]]; then
        case "$RUNNER_OS" in
            Linux)   PLATFORM="linux"   ;;
            macOS)   PLATFORM="macos"   ;;
            Windows) PLATFORM="windows" ;;
            *) echo "Unsupported RUNNER_OS: $RUNNER_OS. Pass --platform." >&2; exit 1;;
        esac
    else
        case "$(uname -s)" in
            Linux*)               PLATFORM="linux"   ;;
            Darwin*)              PLATFORM="macos"   ;;
            MINGW*|CYGWIN*|MSYS*) PLATFORM="windows" ;;
            *) echo "Cannot detect platform from uname -s. Pass --platform." >&2; exit 1;;
        esac
    fi
fi

if [[ -z "$ARCH" ]]; then
    if [[ "$PLATFORM" == "macos" ]]; then
        ARCH="universal"
    elif [[ -n "${RUNNER_ARCH:-}" ]]; then
        case "$RUNNER_ARCH" in
            X86)   ARCH="x86_32" ;;
            X64)   ARCH="x86_64" ;;
            ARM64) ARCH="arm64"  ;;
            *) echo "Unsupported RUNNER_ARCH: $RUNNER_ARCH. Pass --arch." >&2; exit 1;;
        esac
    else
        case "$(uname -m)" in
            x86_64|amd64)  ARCH="x86_64" ;;
            aarch64|arm64) ARCH="arm64"  ;;
            i386|i686)     ARCH="x86_32" ;;
            *) echo "Cannot detect arch from uname -m. Pass --arch." >&2; exit 1;;
        esac
    fi
fi

# --- Compute archive suffix and in-archive binary path ---

if $MONO; then
    case "$PLATFORM" in
        windows)
            case "$ARCH" in
                x86_32) suffix="mono_win32" ;;
                x86_64) suffix="mono_win64" ;;
                arm64)  suffix="mono_windows_arm64" ;;
                *) echo "Unsupported Windows arch: $ARCH" >&2; exit 1;;
            esac
            inner_bin="Godot_v${VERSION}_${suffix}/Godot_v${VERSION}_${suffix}.exe"
            ;;
        linux)
            case "$ARCH" in
                x86_32|x86_64|arm64) suffix="mono_linux_${ARCH}" ;;
                *) echo "Unsupported Linux arch: $ARCH" >&2; exit 1;;
            esac
            inner_bin="Godot_v${VERSION}_${suffix}/Godot_v${VERSION}_mono_linux.${ARCH}"
            ;;
        macos)
            if [[ "$ARCH" != "universal" ]]; then
                echo "Unsupported macOS arch: $ARCH" >&2; exit 1
            fi
            suffix="mono_macos.universal"
            inner_bin="Godot_mono.app/Contents/MacOS/Godot"
            ;;
        *) echo "Unsupported platform: $PLATFORM" >&2; exit 1;;
    esac
else
    case "$PLATFORM" in
        windows)
            case "$ARCH" in
                x86_32) suffix="win32.exe" ;;
                x86_64) suffix="win64.exe" ;;
                arm64)  suffix="windows_arm64.exe" ;;
                *) echo "Unsupported Windows arch: $ARCH" >&2; exit 1;;
            esac
            inner_bin="Godot_v${VERSION}_${suffix}"
            ;;
        linux)
            case "$ARCH" in
                x86_32|x86_64|arm64) suffix="linux.${ARCH}" ;;
                *) echo "Unsupported Linux arch: $ARCH" >&2; exit 1;;
            esac
            inner_bin="Godot_v${VERSION}_${suffix}"
            ;;
        macos)
            if [[ "$ARCH" != "universal" ]]; then
                echo "Unsupported macOS arch: $ARCH" >&2; exit 1
            fi
            suffix="macos.universal"
            inner_bin="Godot.app/Contents/MacOS/Godot"
            ;;
        *) echo "Unsupported platform: $PLATFORM" >&2; exit 1;;
    esac
fi

# --- Download and extract ---

mkdir -p "$DEST"
# Use `pwd -W` under MINGW/Cygwin/MSYS so the path stays Windows-style
# (e.g. D:/a/.../godot) and remains usable by non-Bash consumers like PowerShell.
case "$(uname -s)" in
    MINGW*|CYGWIN*|MSYS*) DEST="$(cd "$DEST" && pwd -W)" ;;
    *)                    DEST="$(cd "$DEST" && pwd)"    ;;
esac

archive_file="Godot_v${VERSION}_${suffix}.zip"
url="https://github.com/godotengine/godot-builds/releases/download/${VERSION}/${archive_file}"

cd "$DEST"
echo "Downloading ${url}"
curl -fL -o godot.zip "$url"
unzip -qo godot.zip
rm godot.zip

# --- Validate and export ---

bin="$DEST/${inner_bin}"
if [[ ! -f "$bin" ]]; then
    echo "Expected binary not found: $bin" >&2
    exit 1
fi

echo "GODOT=$bin"
echo "GODOT_VERSION=$VERSION"
if [[ -n "${GITHUB_ENV:-}" ]]; then
    echo "GODOT=$bin" >> "$GITHUB_ENV"
    echo "GODOT_VERSION=$VERSION" >> "$GITHUB_ENV"
fi
