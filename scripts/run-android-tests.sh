#!/bin/bash
# Run unit tests on a local Android device.

# Configuration
TEST_TIMEOUT=120  # seconds
INSTALL_RETRIES=5
LAUNCH_RETRIES=5
LOCKSCREEN_RETRIES=20
PID_RETRIES=10
LOGCAT_FILTERS="Godot,godot,sentry-godot,sentry-native"
EXPORT_PRESET="Android Tests"

# Launch configuration
PACKAGE="io.sentry.godot.project"
ACTIVITY="com.godot.game.GodotApp"
COMPONENT="$PACKAGE/$ACTIVITY"

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Run unit tests on a local Android device."
    echo ""
    echo "OPTIONS:"
    echo "  -v, --verbose    Enable additional output"
    echo "  -h, --help       Display this help message"
    echo ""
    echo "PREREQUISITES:"
    echo "  - Godot Engine 4.5 or later with Android export templates installed"
    echo "  - Android SDK with ADB (Android Debug Bridge) tools"
    echo "  - Android device connected and authorized for debugging"
    echo "  - Export preset for Android named 'Android Tests'"
    echo "    (see exports/export_presets.cfg)."
    echo ""
    echo "ENVIRONMENT VARIABLES:"
    echo "  GODOT           Path to Godot executable (if not in PATH)"
    echo "  ANDROID_HOME    Path to Android SDK (or set up Android SDK in Godot settings)"
    echo "  JAVA_HOME       Path to Java 17 (or set up Java SDK in Godot settings)"
}

# Formatted output
highlight() { printf '\033[1;34m%s\033[0m\n' "$1"; }
msg()       { printf '\033[1m%s\033[0m\n' "$1"; }
error()     { printf '\033[1;31m%s\033[0m\n' "$1"; }
warning()   { printf '\033[1;33m%s\033[0m\n' "$1"; }
success()   { printf '\033[1;32m%s\033[0m\n' "$1"; }
github()    { [ "$GITHUB_ACTIONS" = "true" ] && printf '%s\n' "$1"; }
blankline() { printf ' \n'; }

# Check exit code of previous command and abort with message if non-zero
abort_on_error() {
    local exit_code=$?
    if [ $exit_code -ne 0 ]; then
        error "$1 (exit code $exit_code). Aborting."
        exit $exit_code
    fi
}

# Exit with code and message
abort() {
    error "$1. Aborting."
    if [ $# -ge 2 ]; then
        exit $2
    else
        exit 1
    fi
}

# Parse command line options
verbose=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose|-v)
            verbose=true
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        -*)
            error "Unknown option: $1"
            usage
            exit 1
            ;;
        *)
            error "Unexpected argument: $1"
            usage
            exit 1
            ;;
    esac
done

highlight "Exporting project..."

# Export project to "exports/android.apk".
godot=$(command -v godot || echo "$GODOT")
if [ -z "$godot" ] || [ ! -x "$godot" ]; then
    abort "Godot executable not found. Please ensure 'godot' is in PATH or set GODOT environment variable."
fi

github "::group::Export log"

"$godot" --verbose --headless --path project --install-android-build-template --export-debug "$EXPORT_PRESET" ../exports/android.apk
export_err=$?

github "::endgroup::"

if [ $export_err -ne 0 ]; then
    warning "Godot export process returned an error. Proceeding anyway..."
fi

# Check if APK was exported successfully
if [ ! -f "./exports/android.apk" ]; then
    abort "APK file not found at ./exports/android.apk. Export failed! Aborting..."
fi

# Install APK (allow multiple attempts)
blankline
highlight "Installing APK..."
adb kill-server 2>/dev/null
for i in $(seq 1 $INSTALL_RETRIES); do
    msg "Waiting for Android device..."
    if adb wait-for-device && adb install -r ./exports/android.apk; then
        break
    elif [ $i -eq $INSTALL_RETRIES ]; then
        abort "Failed to install APK after $INSTALL_RETRIES attempts"
    else
        error "Install attempt $i failed, retrying..."
        sleep 1
    fi
done

# Enable Sentry Android output if verbose
if [ "$verbose" = true ]; then
      LOGCAT_FILTERS="$LOGCAT_FILTERS,Sentry"
fi

# Run tests on device
run_tests() {
    local tests=$1

    # Wait for device lockscreen to be unlocked
    for i in $(seq 1 $LOCKSCREEN_RETRIES); do
        local lock_state=$(adb shell dumpsys window | grep mDreamingLockscreen)
        if [[ "$lock_state" == *"mDreamingLockscreen=false"* ]]; then
            msg "Device lockscreen is unlocked and ready"
            break
        elif [ $i -eq $LOCKSCREEN_RETRIES ]; then
            error "Device lockscreen still active after $LOCKSCREEN_RETRIES attempts"
            return 1
        fi
        warning "Device lockscreen is active, please unlock it..."
        sleep 2
    done

    highlight "Launching APK..."
    for i in $(seq 1 $LAUNCH_RETRIES); do
        # -W: wait to complete, -S: force stop the app before starting activity
        launch_output=$(adb shell am start -W -S -n $COMPONENT --es SENTRY_TEST 1 --es SENTRY_TEST_INCLUDE "$tests" 2>&1)
        launch_err=$?
        printf "%s\n" "$launch_output"

        if [[ $launch_err -ne 0 ]]; then
            error "Launch attempt failed with code: $launch_err"
        elif [[ "$launch_output" != *"LaunchState: COLD"* ]]; then
            error "Expected COLD launch but got different launch state from output"
        elif [[ "$launch_output" != *"Activity: $COMPONENT"* ]]; then
            error "Expected activity '$COMPONENT' but got different activity from launch output"
        else
            # Success
            break
        fi

        if [[ $i -eq $LAUNCH_RETRIES ]]; then
            error "Failed to launch APK after $LAUNCH_RETRIES attempts"
            return 1
        else
            error "Launch attempt $i failed, retrying..."
            adb shell am force-stop "$PACKAGE" 2>/dev/null || true
            adb shell pm clear "$PACKAGE" 2>/dev/null || true
            sleep 2
        fi
    done

    # Get PID
    local pid=""
    for i in $(seq 1 $PID_RETRIES); do
        pid=$(adb shell pidof $PACKAGE)
        if [ -n "$pid" ]; then
            break
        fi
        sleep 1
    done

    if [ -z "$pid" ]; then
        error "Failed to get PID of the app"
        return 1
    fi

    echo "PID: $pid"

    # Start logcat, streaming to stdout and monitoring for completion
    highlight "Reading logs..."
    local exit_code=1  # Default general failure
    local clean_exit=0
    local logcat_cmd="timeout $TEST_TIMEOUT adb logcat --pid=$pid -s $LOGCAT_FILTERS"

    # Function to monitor Android app process and kill logcat if it dies
    monitor_app() {
        while true; do
            local app_pid=$(adb shell pidof $PACKAGE 2>/dev/null || echo "")
            if [ -z "$app_pid" ]; then
                sleep 10  # start a timer to kill logcat
                warning "App died, killing logcat"
                pkill --full "$logcat_cmd" 2>/dev/null || true
                break
            fi
            sleep 1
        done
    }

    # Start monitoring in background
    monitor_app &
    local monitor_pid=$!

    # Process logcat output
    while IFS= read -r line; do
        echo "$line"

        case "$line" in
            # Check for test run completion
            *">>> Test run complete with code:"*)
                exit_code=$(echo "$line" | sed 's/.*>>> Test run complete with code: \([0-9]*\).*/\1/')
                # Not quitting yet -- waiting for Godot to terminate.
                ;;
            # Check Godot exit condition
            *"OnGodotTerminating"*)
                clean_exit=1
                timeout 2 cat || true # Continue reading for a bit in case there are remaining logs
                break
                ;;
        esac
    done < <($logcat_cmd)

    # Check if never finished
    if [ $exit_code -eq 1 ]; then
        error "Test run was interrupted or failed to complete properly!"
    fi

    # Kill app monitor
    kill $monitor_pid 2>/dev/null || true
    wait $monitor_pid 2>/dev/null || true

    # Check if process still running
    local current_pid=$(adb shell pidof $PACKAGE 2>/dev/null || echo "")
    if [ -n "$current_pid" ]; then
        if [ $exit_code -eq 0 ]; then
            exit_code=88
        fi
        error "Godot app process still running"
        adb shell am force-stop $PACKAGE
        # Wait for process to quit
        while adb shell "[ -d /proc/$current_pid ]" >/dev/null 2>&1; do
          sleep 1
        done
    # Check if not exited cleanly
    elif [ $clean_exit -eq 0 ]; then
        warning "Unclean exit detected. Godot possibly crashed."
    fi

    msg "Test run finished with code: $exit_code"

    return $exit_code
}


# Discover isolated test suites and add normal suites as first item
blankline
highlight "Looking for isolated test suites..."
TEST_PATHS=("res://test/suites/")
TEST_PATHS+=($(find project/test/isolated -name "test_*.gd" -type f | sort))

# Check if any isolated tests were found
if [ ${#TEST_PATHS[@]} -eq 1 ]; then
    abort "No isolated test files found in project/test/isolated"
fi

msg "Found $((${#TEST_PATHS[@]} - 1)) isolated test suites."

OVERALL_EXIT_CODE=0
FAILED_TESTS=()
PASSED_TESTS=()

# For each test path perform a separate run
for test_path in "${TEST_PATHS[@]}"; do
    if [[ "$test_path" == "res://test/suites/" ]]; then
        # First item is the normal test suites
        godot_path="$test_path"
    else
        # Convert file path to Godot resource path for isolated tests
        godot_path="res://${test_path#project/}"
    fi

    blankline
    highlight "Running tests: $godot_path"
    github "::group::Test log $godot_path"

    run_tests "$godot_path"
    test_exit_code=$?

    github "::endgroup::"

    if [ $test_exit_code -eq 0 ]; then
        PASSED_TESTS+=("$godot_path")
        success "✓ PASSED: $godot_path"
    else
        FAILED_TESTS+=("$godot_path")
        error "✗ FAILED: $godot_path (exit code: $test_exit_code)"
        OVERALL_EXIT_CODE=100
    fi

    # Small delay between runs
    sleep 1
done

# Summary
blankline
highlight "Final Results"
msg "Passed: ${#PASSED_TESTS[@]}"
msg "Failed: ${#FAILED_TESTS[@]}"

if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
    error "Failed tests:"
    for failed_test in "${FAILED_TESTS[@]}"; do
        error "  - $failed_test"
    done
fi

highlight "Test execution finished with code: $OVERALL_EXIT_CODE"
exit $OVERALL_EXIT_CODE
