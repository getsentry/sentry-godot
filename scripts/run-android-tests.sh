#!/bin/bash
#
# Run unit tests on a local Android device.
#
# Prerequisites:
# - Godot Engine 4.5 or later with Android export templates installed
# - Android SDK with ADB (Android Debug Bridge) tools
# - Android device connected and authorized for debugging

# Configuration
TEST_TIMEOUT=120  # seconds
INSTALL_RETRIES=5
LOCKSCREEN_RETRIES=20

# Formatted output
highlight() { echo -e "\033[1;34m$1\033[0m"; }
msg() { echo -e "\033[1m$1\033[0m"; }
error() { echo -e "\033[1;31m$1\033[0m"; }
success() { echo -e "\033[1;32m$1\033[0m"; }

highlight "Exporting project..."

# Export project to "exports/android.apk".
godot=$(command -v godot || echo "$GODOT")
if [ -z "$godot" ] || [ ! -x "$godot" ]; then
    error "Godot executable not found. Please ensure 'godot' is in PATH or set GODOT environment variable."
    exit 1
fi
"$godot" --path project --headless --install-android-build-template --export-debug "Android CI" ../exports/android.apk

# Install APK (allow multiple attempts)
highlight "Installing APK..."
adb kill-server 2>/dev/null
for i in $(seq 1 $INSTALL_RETRIES); do
	msg "Waiting for Android device..."
    if adb wait-for-device && adb install ./exports/android.apk; then
        break
    elif [ $i -eq $INSTALL_RETRIES ]; then
        msg "Failed to install APK after $INSTALL_RETRIES attempts"
        exit 1
    else
        error "Install attempt $i failed, retrying..."
        sleep 1
    fi
done


# Run tests on device
run_tests() {
	local tests=$1

	# Wait for device lockscreen to be unlocked
	for i in $(seq 1 $LOCKSCREEN_RETRIES); do
		local lock_state=$(adb shell dumpsys window | grep mDreamingLockscreen)
		if echo "$lock_state" | grep -q "mDreamingLockscreen=false"; then
			msg "Device lockscreen is unlocked and ready"
			break
		elif [ $i -eq $LOCKSCREEN_RETRIES ]; then
			error "Device lockscreen still active after $LOCKSCREEN_RETRIES attempts. Aborting."
			exit 1
		fi
		msg "Device lockscreen is active, please unlock it..."
		sleep 2
	done

	highlight "Launching APK..."
	adb shell am start -n io.sentry.godot.project/com.godot.game.GodotApp --es SENTRY_TEST 1 --es SENTRY_TEST_INCLUDE "$tests"

	# Get PID
	sleep 1
	local pid=$(adb shell pidof io.sentry.godot.project)
	if [ -z "$pid" ]; then
	    error "Failed to get PID of the app"
	    return 3
	fi

	highlight "Reading logs..."

	# Start logcat, streaming to stdout and monitoring for completion
	local exit_code=1  # Default general failure
	local start_time=$(date +%s)
	local last_pid_check=$start_time

	# Process logcat output
	while IFS= read -r line; do
	    echo "$line"

	    # Check for test completion
	    if echo "$line" | grep -q ">>> Test run complete with code:"; then
	        exit_code=$(echo "$line" | sed 's/.*>>> Test run complete with code: \([0-9]*\).*/\1/')
	        msg "Test run completion detected"
	        break
	    fi

	    # Check timeout
	    local current_time=$(date +%s)
	    local elapsed=$((current_time - start_time))
	    if [ $elapsed -ge $TEST_TIMEOUT ]; then
	        error "Test run timed out after $TEST_TIMEOUT seconds"
	        exit_code=124  # Timeout exit code
	        break
	    fi

	    # Check if process still running (every 5 seconds)
	    if [ $((current_time - last_pid_check)) -ge 5 ]; then
	        local current_pid=$(adb shell pidof io.sentry.godot.project 2>/dev/null || echo "")
	        last_pid_check=$current_time
	        if [ -z "$current_pid" ] || [ "$current_pid" != "$pid" ]; then
	            error "App process has stopped"
	            # Continue reading for a bit in case there are remaining logs
	            timeout 3 cat || true
	            exit_code=2  # Process died without completion
	            break
	        fi
	    fi
	done < <(adb logcat --pid=$pid -s Godot,godot,sentry-godot)

	if [ $exit_code -eq 1 ]; then
	    error "Test run was interrupted or failed to complete properly!"
	fi

	return $exit_code
}


# Discover isolated test suites and add normal suites as first item
TEST_PATHS=("res://test/suites/")
TEST_PATHS+=($(find project/test/isolated -name "test_*.gd" -type f | sort))

highlight "Found $((${#TEST_PATHS[@]} - 1)) isolated tests."

OVERALL_EXIT_CODE=0
FAILED_TESTS=()
PASSED_TESTS=()

# Run each test file separately
for test_path in "${TEST_PATHS[@]}"; do
    if [[ "$test_path" == "res://test/suites/" ]]; then
        # First item is the test suites
        godot_path="$test_path"
    else
        # Convert file path to Godot resource path for isolated tests
        godot_path="res://${test_path#project/}"
    fi

    highlight "\nRunning tests: $godot_path"

    run_tests "$godot_path"
    test_exit_code=$?

    if [ $test_exit_code -eq 0 ]; then
        PASSED_TESTS+=("$godot_path")
        success "✓ PASSED: $godot_path"
    else
        FAILED_TESTS+=("$godot_path")
        error "✗ FAILED: $godot_path (exit code: $test_exit_code)"
        OVERALL_EXIT_CODE=1
    fi

    # Small delay between tests
    sleep 2
done

# Summary
highlight "\nTest Summary"
msg "Passed: ${#PASSED_TESTS[@]}"
msg "Failed: ${#FAILED_TESTS[@]}"

if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
    error "Failed tests:"
    for failed_test in "${FAILED_TESTS[@]}"; do
        error "  - $failed_test"
    done
fi

highlight "Overall test run finished with exit code: $OVERALL_EXIT_CODE"
exit $OVERALL_EXIT_CODE
