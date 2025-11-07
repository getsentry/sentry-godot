#!/bin/bash

usage() {
    echo "Usage: $0 GODOT_VERSION"
    echo ""
    echo "Download and install Android build template."
    echo "Tip: Call from repo root."
    echo ""
    echo "Arguments:"
    echo "  GODOT_VERSION                  Godot version to install (e.g., 4.2.1-stable)"
    echo ""
    echo "Example:"
    echo "  $0 4.5.1-stable"
    exit 1
}

# Enable exit on error
set -e

# Parse opts
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            ;;
        -*)
            echo "Error: Unknown option '$1'"
            echo ""
            usage
            ;;
        *)
            # First non-option argument is GODOT_VERSION
            if [[ -z "$godot_version" ]]; then
                godot_version="$1"
            else
                echo "Error: Unexpected argument '$1'"
                echo ""
                usage
            fi
            shift
            ;;
    esac
done

# Check if required argument is provided
if [[ -z "$godot_version" ]]; then
    echo "Error: GODOT_VERSION argument is required"
    echo ""
    usage
fi

echo "Installing Android template for Godot version: $godot_version"

# Download Godot templates
archive_file=Godot_v${godot_version}_export_templates.tpz
url=https://github.com/godotengine/godot-builds/releases/download/${godot_version}/${archive_file}
echo "Downloading templates from: $url"
curl -L -o templates.zip "${url}"

echo "Extracting Android source template..."
rm -f exports/android_source.zip
unzip -j templates.zip templates/android_source.zip -d exports/
rm templates.zip

echo "Installing Android Gradle project..."
rm -rf project/android/
mkdir -p project/android/build
unzip exports/android_source.zip -d project/android/build

echo "Adding version metadata..."
echo "../exports/android_source.zip [$(md5sum exports/android_source.zip | cut -d' ' -f1)]" > project/android/.build_version

echo "Android build template deployed successfully!"
