#!/usr/bin/env bash
#
# Publish a Godot asset to the Godot Asset Library.
# Reads asset metadata from a YAML file, resolves the download URL
# from a GitHub Release, and submits an edit to the AssetLib API.
#
# Usage: publish-assetlib.sh [-n|--dry-run] <metadata.yaml> <version> <godot_version>
#
# Environment variables:
#   ASSETLIB_USERNAME  - AssetLib account username (not required in dry-run mode)
#   ASSETLIB_PASSWORD  - AssetLib account password (not required in dry-run mode)
#
# Requirements: yq, jq, gh (authenticated), curl

set -euo pipefail

ASSETLIB_API="https://godotengine.org/asset-library/api"

# --- Parse arguments ---

DRY_RUN=false
PARAMS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        -n|--dry-run) DRY_RUN=true; shift;;
        -*) echo "Unknown option: $1" >&2; exit 1;;
        *) PARAMS+=("$1"); shift;;
    esac
done

if [[ ${#PARAMS[@]} -ne 3 ]]; then
    echo "Usage: publish-assetlib.sh [-n|--dry-run] <metadata.yaml> <version> <godot_version>" >&2
    exit 1
fi

METADATA_FILE="${PARAMS[0]}"
VERSION="${PARAMS[1]}"
GODOT_VERSION="${PARAMS[2]}"

if [[ ! -f "$METADATA_FILE" ]]; then
    echo "Error: Metadata file not found: $METADATA_FILE" >&2
    exit 1
fi

# --- Read YAML and convert to JSON ---

ASSET_JSON=$(yq -o=json '.' "$METADATA_FILE")

ASSET_ID=$(echo "$ASSET_JSON" | jq -r '.asset_id')
ASSET_PREFIX=$(echo "$ASSET_JSON" | jq -r '.download_asset_prefix')
REPO=$(echo "$ASSET_JSON" | jq -r '.browse_url | ltrimstr("https://github.com/")')

echo "Asset: $(echo "$ASSET_JSON" | jq -r '.title') (ID: $ASSET_ID)"
echo "Version: $VERSION, Godot: $GODOT_VERSION"

# --- Resolve download URL from GitHub Release assets ---

# Substitute {version} in the prefix before matching.
ASSET_PREFIX="${ASSET_PREFIX//\{version\}/$VERSION}"

DOWNLOAD_URL=$(gh release view "$VERSION" --repo "$REPO" \
    --json assets --jq ".assets[] | select(.name | startswith(\"${ASSET_PREFIX}\")) | .url")

if [[ -z "$DOWNLOAD_URL" ]]; then
    echo "Error: No release asset matching prefix '$ASSET_PREFIX' in release $VERSION" >&2
    exit 1
fi

echo "Download URL: $DOWNLOAD_URL"

# --- Build payload ---

# Substitute {version} placeholders in all string fields, then inject dynamic fields.
PAYLOAD=$(echo "$ASSET_JSON" | \
    jq --arg version "$VERSION" \
       --arg godot "$GODOT_VERSION" \
       --arg download "$DOWNLOAD_URL" \
       'walk(if type == "string" then gsub("\\{version\\}"; $version) else . end)
        | . + {
            version_string: $version,
            godot_version: $godot,
            download_commit: $download
          }
        | del(.asset_id, .download_asset_prefix)')

# --- Dry-run mode: print payload and exit ---

if [[ "$DRY_RUN" == true ]]; then
    echo ""
    echo "--- Dry run: payload that would be submitted ---"
    echo "$PAYLOAD" | jq .
    exit 0
fi

# --- Validate credentials ---

if [[ -z "${ASSETLIB_USERNAME:-}" || -z "${ASSETLIB_PASSWORD:-}" ]]; then
    echo "Error: ASSETLIB_USERNAME and ASSETLIB_PASSWORD must be set" >&2
    exit 1
fi

# --- Login ---

echo "Logging in to AssetLib..."
LOGIN_RESPONSE=$(curl -sS --fail-with-body "$ASSETLIB_API/login" \
    -d "$(jq -n --arg u "$ASSETLIB_USERNAME" --arg p "$ASSETLIB_PASSWORD" \
        '{username: $u, password: $p}')" \
    -H "Content-Type: application/json")

TOKEN=$(echo "$LOGIN_RESPONSE" | jq -r '.token')
if [[ -z "$TOKEN" || "$TOKEN" == "null" ]]; then
    echo "Error: Login failed" >&2
    echo "$LOGIN_RESPONSE" >&2
    exit 1
fi

# Ensure we log out even if the edit request fails.
logout() { curl -sS --fail-with-body "$ASSETLIB_API/logout" \
    -d "$(jq -n --arg t "$TOKEN" '{token: $t}')" \
    -H "Content-Type: application/json" > /dev/null 2>&1 || true; }
trap logout EXIT

# --- Submit edit ---

PAYLOAD=$(echo "$PAYLOAD" | jq --arg token "$TOKEN" '. + {token: $token}')

echo "Submitting edit for asset $ASSET_ID..."
EDIT_RESPONSE=$(curl -sS --fail-with-body "$ASSETLIB_API/asset/$ASSET_ID" \
    -d "$PAYLOAD" \
    -H "Content-Type: application/json")

EDIT_ID=$(echo "$EDIT_RESPONSE" | jq -r '.id')
if [[ -z "$EDIT_ID" || "$EDIT_ID" == "null" ]]; then
    echo "Error: Failed to submit edit" >&2
    echo "$EDIT_RESPONSE" >&2
    exit 1
fi

echo "Created edit $EDIT_ID for asset $ASSET_ID"
