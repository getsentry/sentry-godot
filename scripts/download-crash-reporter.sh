#!/usr/bin/env bash
#
# Download the Sentry Desktop Crash Reporter release pinned in
# modules/sentry-crash-reporter.properties into the addon's crash_reporter folder.
# Run with --help for usage.

set -euo pipefail

usage() {
    cat <<'USAGE' >&2
Usage: download-crash-reporter.sh [options]

Download the pinned crash reporter executables for Linux and Windows (x86_64
and arm64), with a .gdignore.

Options:
  --output <dir>     Destination folder (default: project/addons/sentry/crash_reporter)
  -h, --help         Show this help
USAGE
    exit "$1"
}

# Usage: die <message> [exit-code]
die() {
    echo "Error: $1" >&2
    exit "${2:-1}"
}

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT="$REPO_ROOT/project/addons/sentry/crash_reporter"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output) OUTPUT="${2%/}"; shift 2;;
        -h|--help) usage 0;;
        *) echo "Unknown argument: $1" >&2; usage 1;;
    esac
done

PROPERTIES="$REPO_ROOT/modules/sentry-crash-reporter.properties"
[[ -f "$PROPERTIES" ]] || die "properties file not found: $PROPERTIES"

VERSION="$(grep '^version=' "$PROPERTIES" | cut -d '=' -f 2 || true)"
REPO="$(grep '^repo=' "$PROPERTIES" | cut -d '=' -f 2 || true)"
[[ -n "$VERSION" && -n "$REPO" ]] || die "version or repo missing in $PROPERTIES"

BASE_URL="$REPO/releases/download/$VERSION/sentry-desktop-crash-reporter-$VERSION"

# Usage: download <platform> <arch> <runtime-id>
#   platform    Godot platform name (linux, windows)
#   arch        Godot architecture name (x86_64, arm64)
#   runtime-id  .NET runtime identifier used in the upstream asset name
download() {
    local platform="$1" arch="$2" rid="$3"
    local dest="$OUTPUT/$platform/$arch"
    local tmp; tmp="$(mktemp -d)"

    echo "Downloading crash reporter $VERSION for $platform/$arch ($rid)"
    mkdir -p "$dest"
    if [[ "$platform" == "windows" ]]; then
        curl -fsSL "$BASE_URL-$rid.zip" -o "$tmp/archive.zip" || die "failed to download $BASE_URL-$rid.zip"
        unzip -qo "$tmp/archive.zip" Sentry.CrashReporter.exe -d "$tmp" || die "Sentry.CrashReporter.exe not found in $rid archive"
        mv "$tmp/Sentry.CrashReporter.exe" "$dest/"
    else
        curl -fsSL "$BASE_URL-$rid.tar.gz" -o "$tmp/archive.tar.gz" || die "failed to download $BASE_URL-$rid.tar.gz"
        tar -xzf "$tmp/archive.tar.gz" -C "$tmp" ./Sentry.CrashReporter || die "Sentry.CrashReporter not found in $rid archive"
        mv "$tmp/Sentry.CrashReporter" "$dest/"
        chmod 755 "$dest/Sentry.CrashReporter"
    fi
    rm -rf "$tmp"
}

main() {
    mkdir -p "$OUTPUT"

    download linux x86_64 linux-x64
    download linux arm64 linux-arm64
    download windows x86_64 win-x64
    download windows arm64 win-arm64

    # Keeps Godot from importing the folder, so export include filters never pack this dir into the PCK.
    touch "$OUTPUT/.gdignore"
    chmod 644 "$OUTPUT/.gdignore"

    echo "Wrote $OUTPUT"
}

main
