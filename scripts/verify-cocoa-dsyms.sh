#!/usr/bin/env bash
# Verify that macOS and iOS extension dSYMs resolve a Cocoa SDK function to its source line.

set -euo pipefail

bin_dir="$(cd "$(dirname "$0")/../project/addons/sentry/bin" && pwd)"

verify_source_line() {
    local platform="$1" library="$2"
    local dsym="$bin_dir/$platform/dSYMs/$library.dSYM"
    local dwarf="$dsym/Contents/Resources/DWARF/$library"
    local address
    echo "Checking $library"
    address=$(nm -arch arm64 -an "$dwarf" | awk '$NF == "_sentrycrashdl_convertBinaryImageUUID" { print $1 }')
    if [[ -z "$address" ]]; then
        echo "Missing Cocoa crash-handler symbol in $dsym" >&2
        return 1
    fi
    dwarfdump --arch=arm64 --lookup="0x$address" "$dsym" |
        grep -E "Line info: file .*SentryCrashUUIDConversion[.]c', line [1-9]"
}

for variant in debug release; do
    verify_source_line macos "libsentry.macos.$variant.dylib"
    verify_source_line ios "libsentry.ios.$variant.arm64.dylib"
    verify_source_line ios "libsentry.ios.$variant.universal.simulator.dylib"
done
