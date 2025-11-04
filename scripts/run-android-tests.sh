#!/bin/bash
#
# Run unit tests on a local Android device.
#
# Prerequisites:
# - Godot Engine 4.5 or later with Android export templates installed
# - Android SDK with ADB (Android Debug Bridge) tools
# - Android device connected and authorized for debugging

set -e

# Export project to "exports/android.apk".
godot=$(command -v godot || echo "$GODOT")
"$godot" --path project --headless --install-android-build-template --export-debug "Android CI" ../exports/android.apk

# Install APK (allow multiple attempts)
echo "Installing APK..."
for i in {1..5}; do
    if adb install ./exports/android.apk; then
        break
    elif [ $i -eq 5 ]; then
        echo "Failed to install APK after 5 attempts"
        exit 1
    else
        echo "Install attempt $i failed, retrying..."
        sleep 1
    fi
done

# Wait for device lockscreen to be unlocked
for i in {1..10}; do
    LOCK_STATE=$(adb shell dumpsys window | grep mDreamingLockscreen)
    if echo "$LOCK_STATE" | grep -q "mDreamingLockscreen=false"; then
        echo "Device lockscreen is unlocked and ready"
        break
    fi
    echo "Device lockscreen is active, please unlock it..."
    sleep 2
done

echo "Launching APK..."
adb shell am start -n io.sentry.godot.project/com.godot.game.GodotApp --es SENTRY_TEST 1 --es SENTRY_TEST_INCLUDE "res://test/suites/"

# Get PID
sleep 1
PID=$(adb shell pidof io.sentry.godot.project)
if [ -z "$PID" ]; then
    echo "Failed to get PID of the app"
    exit 3
fi

echo "Reading logs..."

# Start logcat, streaming to stdout and monitoring for completion
EXIT_CODE=1  # Default general failure
START_TIME=$(date +%s)
LAST_PID_CHECK=$START_TIME

# Process logcat output
while IFS= read -r line; do
    echo "$line"

    # Check for test completion
    if echo "$line" | grep -q ">>> Test run complete with code:"; then
        EXIT_CODE=$(echo "$line" | sed 's/.*>>> Test run complete with code: \([0-9]*\).*/\1/')
        echo "Test completion detected" >&2
        break
    fi

    # Check timeout
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - START_TIME))
    if [ $ELAPSED -ge 120 ]; then
        echo "Testing timed out after 2 minutes" >&2
        EXIT_CODE=124  # Timeout exit code
        break
    fi

    # Check if process still running (every 5 seconds)
    if [ $((CURRENT_TIME - LAST_PID_CHECK)) -ge 5 ]; then
        CURRENT_PID=$(adb shell pidof io.sentry.godot.project 2>/dev/null || echo "")
        LAST_PID_CHECK=$CURRENT_TIME
        if [ -z "$CURRENT_PID" ] || [ "$CURRENT_PID" != "$PID" ]; then
            echo "App process has stopped" >&2
            # Continue reading for a bit in case there are remaining logs
            timeout 3 cat || true
            EXIT_CODE=2  # Process died without completion
            break
        fi
    fi
done < <(adb logcat --pid=$PID -s Godot,godot,sentry-godot)

if [ $EXIT_CODE -eq 1 ]; then
    echo "Test run was interrupted or failed to complete properly!" >&2
fi

echo "Test run completed with exit code: $EXIT_CODE" >&2
exit $EXIT_CODE
