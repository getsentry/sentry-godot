#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat <<EOF >&2
Usage: $(basename "$0") [-h|--help]

Verify that packaged macOS GDExtension and Sentry Cocoa binaries are universal
and use the minimum macOS version declared in SConstruct.
EOF
    exit "$1"
}

die() {
    echo "$1" >&2
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) usage 0;;
        *) echo "Unexpected argument: $1" >&2; usage 1;;
    esac
done

shopt -s nullglob
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
scons_file="$repo_root/SConstruct"
bin_dir="$repo_root/project/addons/sentry/bin/macos"
expected_min_version="$(sed -nE 's/^MACOS_MIN_VERSION = "([^"]+)"$/\1/p' "$scons_file")"

if [[ -z "$expected_min_version" ]]; then
    die "Error: Could not find MACOS_MIN_VERSION in $scons_file"
fi

extension_binaries=("$bin_dir"/libsentry.macos.*.dylib)
if [[ ${#extension_binaries[@]} -eq 0 ]]; then
    die "Error: No macOS GDExtension binaries found in $bin_dir"
fi

for binary in "${extension_binaries[@]}" "$bin_dir/libSentry.dylib"; do
    lipo "$binary" -verify_arch arm64 x86_64
    for architecture in arm64 x86_64; do
        actual_min_version="$(otool -arch "$architecture" -l "$binary" | awk '$1 == "minos" { print $2; exit }')"
        if [[ "$actual_min_version" != "$expected_min_version" ]]; then
            message="Expected $architecture minimum macOS version $expected_min_version, found ${actual_min_version:-none}."
            if [[ "${GITHUB_ACTIONS:-}" == "true" ]]; then
                die "::error file=$binary::$message"
            fi
            die "Error: $binary: $message"
        fi
    done
done
