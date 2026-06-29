#!/usr/bin/env bash
#
# Assemble the Sentry Godot packages (addon, demo project, debug symbols) by
# zipping a prebuilt source tree. Run with --help for usage.

set -euo pipefail

usage() {
    cat <<'EOF' >&2
Usage: package.sh [options]

Assemble the packages from a source tree that contains the demo project and the
built addon (addons/sentry/):
  <name>-<version>.zip                 addon only (addons/sentry/), symbols removed
  <name>-demo-project-<version>.zip    demo project bundled with the addon
  <name>-debug-symbols-<version>.zip   split native debug symbols

Options:
  --source <dir>     Tree with addons/ and the demo project (default: project)
  --output <dir>     Directory to write the zips into (default: out)
  --name <name>      Archive base name (default: sentry-godot)
  --version <ver>    Version tag used in filenames, e.g. 2.0.0+abc1234
                     (default: <VERSION from ./SConstruct>+<git short sha>)
  -h, --help         Show this help
EOF
    exit "$1"
}

# Debug-symbol paths dropped from addon and demo zips.
# Zip globs span '/', so one '*' covers per-arch subdirs.
# Keep patterns platform-specific to avoid matching:
#   - iOS XCFramework binary literally named "libsentry.ios.debug"
#   - .NET pdbs under addons/sentry/dotnet/ are needed for local stack walking
SYMBOL_PATTERNS=(
    "addons/sentry/bin/linux/*.debug"
    "addons/sentry/bin/android/*.debug"
    "addons/sentry/bin/web/*.debug"
    "addons/sentry/bin/windows/*.pdb"
    "addons/sentry/bin/macos/dSYMs/*"
    "addons/sentry/bin/ios/dSYMs/*"
)

# Paths dropped from the demo-project zip (in addition to the symbol patterns).
DEMO_EXCLUDE_PATTERNS=(
    "project.godot"       # re-added sanitized after zipping
    "test/*"              # test suites and harness
    ".*"                  # top-level dotfiles (.godot cache, .vscode, .zed, etc.)
    "android/*"           # Godot Android build template (regenerated on export)
    "addons/gdUnit4/*"    # test framework
    "reports/*"           # local test reports
    "export_presets.cfg"  # local export config
)

# Usage: abort <message> [exit-code]
abort() {
    echo "Error: $1" >&2
    exit "${2:-1}"
}

# Usage: make_zip <zipfile> <items...>
# Runs from $SOURCE so paths are stored tree-relative. Aborts on zip error.
make_zip() {
    local zipfile="$1"; shift
    rm -f "$zipfile"
    local rc=0
    ( cd "$SOURCE" && zip -qr "$zipfile" "$@" ) || rc=$?
    if [[ $rc -ne 0 ]]; then
        abort "zip failed with code $rc ($zipfile)" "$rc"
    fi
}

# Usage: stage_file <zipfile> <src-file> <dest-path>
#   zipfile    archive to append to
#   src-file   file to add (may live outside $SOURCE)
#   dest-path  path it is stored under inside the archive
# Staged via a temp tree so zip records <dest-path> verbatim.
stage_file() {
    local zipfile="$1" src="$2" dest="$3"
    local stage; stage="$(mktemp -d)"
    mkdir -p "$stage/$(dirname "$dest")"
    cp "$src" "$stage/$dest"
    ( cd "$stage" && zip -q "$zipfile" "$dest" )
    rm -rf "$stage"
}

# --- Parse arguments ---

SOURCE="project"
OUTPUT="out"
NAME="sentry-godot"
VERSION=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --source) SOURCE="${2%/}"; shift 2;;
        --output) OUTPUT="${2%/}"; shift 2;;
        --name) NAME="$2"; shift 2;;
        --version) VERSION="$2"; shift 2;;
        -h|--help) usage 0;;
        -*) echo "Unknown option: $1" >&2; usage 1;;
        *) echo "Unexpected argument: $1" >&2; usage 1;;
    esac
done

mkdir -p "$OUTPUT"
OUT_ABS="$(cd "$OUTPUT" && pwd)"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# --- Resolve version ---

# Default version tag is "<SConstruct VERSION>+<git short sha>".
if [[ -z "$VERSION" ]]; then
    sconstruct="$REPO_ROOT/SConstruct"
    ver=""
    [[ -f "$sconstruct" ]] && ver="$(grep '^VERSION =' "$sconstruct" | cut -d '"' -f 2)"
    sha="$(git -C "$REPO_ROOT" rev-parse --short HEAD 2>/dev/null || true)"
    if [[ -z "$ver" || -z "$sha" ]]; then
        abort "could not derive version tag; pass --version <version>+<sha>"
    fi
    VERSION="${ver}+${sha}"
fi

# --- Prepare addon ---

if [[ ! -d "$SOURCE/addons" ]]; then
    abort "addons not found in source tree: $SOURCE/addons"
fi

# Bundled into the addon and demo zips.
LICENSE_ABS="$REPO_ROOT/LICENSE.md"
if [[ ! -f "$LICENSE_ABS" ]]; then
    abort "LICENSE not found: $LICENSE_ABS"
fi

# Fix crashpad_handler permissions;
# workaround for https://github.com/actions/upload-artifact/issues/38
if [[ -d "$SOURCE/addons/sentry/bin" ]]; then
    find "$SOURCE/addons/sentry/bin/" -name "crashpad_handler" -exec chmod 755 "{}" \;
fi

# --- Debug symbols package ---

zipfile="$OUT_ABS/${NAME}-debug-symbols-${VERSION}.zip"
make_zip "$zipfile" addons -i "${SYMBOL_PATTERNS[@]}"
echo "Wrote $zipfile"

# --- Addon package ---

zipfile="$OUT_ABS/${NAME}-${VERSION}.zip"
make_zip "$zipfile" addons/sentry -x "${SYMBOL_PATTERNS[@]}"
stage_file "$zipfile" "$LICENSE_ABS" "addons/sentry/LICENSE.md"
echo "Wrote $zipfile"

# --- Demo project package ---

if [[ ! -f "$SOURCE/project.godot" ]]; then
    abort "project.godot not found in source tree: $SOURCE"
fi

zipfile="$OUT_ABS/${NAME}-demo-project-${VERSION}.zip"
make_zip "$zipfile" . -x "${DEMO_EXCLUDE_PATTERNS[@]}" "${SYMBOL_PATTERNS[@]}"

# Sanitize project.godot: drop [editor_plugins]/[gdunit4] sections, clear DSN.
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
awk '/^\[editor_plugins\]/ { skip=1; next } /^\[gdunit4\]/ { skip=1; next } /^\[/ { skip=0 } !skip' \
    "$SOURCE/project.godot" | sed '/^options\/dsn=/d' > "$tmp/project.godot"
( cd "$tmp" && zip -q "$zipfile" project.godot )

# Include LICENSE.md at the project root for the demo and in addons/sentry/
# so the addon remains licensed if copied out of the demo project.
stage_file "$zipfile" "$LICENSE_ABS" "LICENSE.md"
stage_file "$zipfile" "$LICENSE_ABS" "addons/sentry/LICENSE.md"

echo "Wrote $zipfile"
