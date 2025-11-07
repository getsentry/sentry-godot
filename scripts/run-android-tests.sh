#!/bin/bash
# Run unit tests on a local Android device.

# Configuration
TEST_TIMEOUT=120  # seconds
INSTALL_RETRIES=5
LAUNCH_RETRIES=3
LOCKSCREEN_RETRIES=20
LOGCAT_FILTERS="Godot,godot,sentry-godot,sentry-native"

# Formatted output
highlight() { echo -e "\033[1;34m$1\033[0m"; }
msg() { echo -e "\033[1m$1\033[0m"; }
error() { echo -e "\033[1;31m$1\033[0m"; }
warning() { echo -e "\033[1;33m$1\033[0m"; }
success() { echo -e "\033[1;32m$1\033[0m"; }

# Check exit code of previous command and abort with message if non-zero
abort_on_error() {
    local exit_code=$?
    if [ $exit_code -ne 0 ]; then
        error "$1 (exit code $exit_code). Aborting."
        exit $exit_code
    fi
}

# Exit with code 1 and message
abort() {
	error "$1. Aborting."
	exit 1
}

# Display usage information
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
    echo ""
    echo "ENVIRONMENT VARIABLES:"
    echo "  GODOT           Path to Godot executable (if not in PATH)"
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
    error "Godot executable not found. Please ensure 'godot' is in PATH or set GODOT environment variable."
    exit 1
fi
"$godot" --path project --headless --install-android-build-template --export-debug "Android CI" ../exports/android.apk
abort_on_error "Godot export failed"

# Install APK (allow multiple attempts)
highlight "\nInstalling APK..."
adb kill-server 2>/dev/null
for i in $(seq 1 $INSTALL_RETRIES); do
	msg "Waiting for Android device..."
    if adb wait-for-device && adb install ./exports/android.apk; then
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
		if echo "$lock_state" | grep -q "mDreamingLockscreen=false"; then
			msg "Device lockscreen is unlocked and ready"
			break
		elif [ $i -eq $LOCKSCREEN_RETRIES ]; then
			abort "Device lockscreen still active after $LOCKSCREEN_RETRIES attempts"
		fi
		warning "Device lockscreen is active, please unlock it..."
		sleep 2
	done

	highlight "Launching APK..."
	for i in $(seq 1 $LAUNCH_RETRIES); do
		adb shell am start -n io.sentry.godot.project/com.godot.game.GodotApp --es SENTRY_TEST 1 --es SENTRY_TEST_INCLUDE "$tests"
		if [ $? -eq 0 ]; then
			# Success
			break
		elif [ $i -eq $LAUNCH_RETRIES ]; then
			abort "Failed to launch APK after $LAUNCH_RETRIES attempts"
		else
			error "Launch attempt $i failed, retrying..."
			sleep 1
		fi
	done

	# Get PID
	sleep 2
	local pid=$(adb shell pidof io.sentry.godot.project)
	if [ -z "$pid" ]; then
	    error "Failed to get PID of the app"
	    return 3
	fi

	# Start logcat, streaming to stdout and monitoring for completion
	highlight "Reading logs..."

	local exit_code=1  # Default general failure
	local clean_exit=0

	# Process logcat output
	while IFS= read -r line; do
	    echo "$line"

	    # Check for test run completion
	    if echo "$line" | grep -q ">>> Test run complete with code:"; then
	        exit_code=$(echo "$line" | sed 's/.*>>> Test run complete with code: \([0-9]*\).*/\1/')
			# Not quitting yet -- waiting for Godot to terminate.
	    fi

	    # Check Godot exit condition
		if echo "$line" | grep -q "OnGodotTerminating"; then
			clean_exit=1
            timeout 2 cat || true  # Continue reading for a bit in case there are remaining logs
	        break
	    fi
	done < <(timeout $TEST_TIMEOUT adb logcat --pid=$pid -s $LOGCAT_FILTERS)

	# Check if never finished
	if [ $exit_code -eq 1 ]; then
  		error "Test run was interrupted or failed to complete properly!"
	fi

    # Check if process still running
    local current_pid=$(adb shell pidof io.sentry.godot.project 2>/dev/null || echo "")
    if [ -n "$current_pid" ] && [ "$current_pid" = "$pid" ]; then
    	if [ $exit_code -eq 0 ]; then
        	exit_code=88
     	fi
        error "Godot app process still running"
    # Check if not exited cleanly
	elif [ $clean_exit -eq 0 ]; then
		warning "Unclean exit detected. Godot possibly crashed."
    fi

	msg "Test run finished with code: $exit_code"

	return $exit_code
}


# Discover isolated test suites and add normal suites as first item
TEST_PATHS=("res://test/suites/")
highlight "\nLooking for isolated test suites..."
TEST_PATHS+=($(find project/test/isolated -name "test_*.gd" -type f | sort))
abort_on_error "Failed to find isolated test suites"
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

    highlight "\nRunning tests: $godot_path"

    run_tests "$godot_path"
    test_exit_code=$?

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
highlight "\nTest Summary"
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
