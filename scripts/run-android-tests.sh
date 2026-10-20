#!/bin/bash
# Run unit tests on a local Android device.

# Configuration
TEST_TIMEOUT=120  # seconds
INSTALL_RETRIES=5
LAUNCH_RETRIES=5
LOCKSCREEN_RETRIES=20
PID_RETRIES=10
EXIT_INFO_RETRIES=5
CRASH_DRAIN_SECONDS=5
LOGCAT_PIDS=()
LOG_STREAM_PID=""
LOGCAT_FILTERS="Godot,godot,sentry-godot,sentry-native"
EXPORT_PRESET="Android Tests"

# Launch configuration
PACKAGE="io.sentry.godot.project"
MAIN_ACTIVITY="com.godot.game.GodotApp"

# Special exit codes
EXIT_PROCESS_STILL_RUNNING_AFTER_TESTS=88
EXIT_UNIT_TEST_RUN_CRASHED=89

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

exit_code_reason() {
    case "$1" in
        1) echo "runner error" ;;
        "$EXIT_PROCESS_STILL_RUNNING_AFTER_TESTS") echo "process still running after timeout" ;;
        "$EXIT_UNIT_TEST_RUN_CRASHED") echo "crash or unclean exit" ;;
        100) echo "test failures" ;;
        101) echo "test warnings" ;;
        104) echo "no tests found" ;;
        105) echo "tests did not run" ;;
        106) echo "script errors" ;;
        *) echo "unknown failure" ;;
    esac
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
# Prefer the GODOT env var over `godot` in PATH.
if [ -n "$GODOT" ] && [ -x "$GODOT" ]; then
    godot="$GODOT"
else
    godot=$(command -v godot)
fi
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

# Need API 30
api_level=$(adb shell getprop ro.build.version.sdk | tr -d '\r')
if [[ ! "$api_level" =~ ^[0-9]+$ ]] || [ "$api_level" -lt 30 ]; then
    abort "Dirty-exit verification requires Android API 30 or newer (device API: $api_level)" "$EXIT_UNIT_TEST_RUN_CRASHED"
fi

mkdir -p tests/results/android || exit 1
RESULTS_DIR=$(mktemp -d tests/results/android/run-XXXXXX) || exit 1

# Resolve the launcher from the device instead of hardcoding it: Godot 4.6+ puts an exported
# alias in front of MAIN_ACTIVITY, while older versions expose MAIN_ACTIVITY directly.
resolve_launcher() {
    local resolved
    resolved=$(adb shell cmd package resolve-activity --brief "$PACKAGE" 2>/dev/null | tail -n 1 | tr -d '\r')
    if [[ "$resolved" == "$PACKAGE/"* ]]; then
        echo "$resolved"
    else
        echo "$PACKAGE/$MAIN_ACTIVITY"
    fi
}

stop_logcat() {
    local collector
    for collector in "${LOGCAT_PIDS[@]}"; do
        kill "$collector" 2>/dev/null || true
        wait "$collector" 2>/dev/null || true
    done
    LOGCAT_PIDS=()
    if [ -n "$LOG_STREAM_PID" ]; then
        wait "$LOG_STREAM_PID" 2>/dev/null || true
        LOG_STREAM_PID=""
    fi
}
trap stop_logcat EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

# Find the app's PID in startup messages or native crash reports in logcat logs.
find_pid_in_logs() {
    local app_log=$1 crash_log=$2 pid
    local package_pattern=${PACKAGE//./\\.}

    # Startup messages contain 'Start proc PID:PACKAGE/USER'.
    pid=$(sed -nE "s|.*[[:space:]]Start proc ([0-9]+):${package_pattern}/.*|\1|p" "$app_log" | head -n 1)
    if [ -z "$pid" ]; then
        # Native crash headers contain 'pid: PID, ... >>> PACKAGE <<<'.
        pid=$(sed -nE "/>>> ${package_pattern} <<</s|^([^[:space:]]+[[:space:]]+){5}DEBUG[[:space:]]+:.*[[:space:]]pid: ([0-9]+),.*|\2|p" \
            "$crash_log" | head -n 1)
    fi
    printf '%s\n' "$pid"
}

# Print the app's exit record since launch, matching the PID if known.
# Return failure unless exactly one record matches.
find_exit_info() {
    local history=$1 since=$2 pid=$3
    # Select the unique multiline exit record matching the package, launch time, and optional PID.
    awk -v package="$PACKAGE" -v since="$since" -v pid="$pid" '
        /ApplicationExitInfo #[0-9]+:/ {
            block = $0 "\n"
            if (getline <= 0 || $1 !~ /^timestamp=/ || $3 !~ /^pid=[0-9]+$/) next
            timestamp = substr($1, 11) " " $2
            record_pid = substr($3, 5)
            block = block $0 "\n"
            if (getline <= 0 || $1 != "process=" package) next
            block = block $0 "\n"
            if (getline > 0) block = block $0 "\n"
            if (timestamp ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}$/ &&
                "x" timestamp >= "x" since && (!pid || record_pid == pid)) {
                matched = block
                count++
            }
        }
        END { if (count == 1) printf "%s", matched; else exit 1 }
    ' "$history"
}

# Filter a logcat crash log in threadtime format, printing matching lines unchanged.
# Match the app's PID directly or inside native reports written by crash_dump.
filter_crash_log() {
    local log_file=$1 pid=$2
    # Print app crash messages and matching native reports, preserving report headers.
    awk -v pid="$pid" -v package="$PACKAGE" '
        $6 == "DEBUG" {
            dumper = $3
            if (index($0, "*** *** ***")) { header[dumper] = ""; matched[dumper] = 0 }
            header[dumper] = header[dumper] $0 "\n"
            if (index($0, "pid: " pid ",") && index($0, ">>> " package " <<<")) {
                printf "%s", header[dumper]
                matched[dumper] = 1
            } else if (matched[dumper]) print
            next
        }
        $3 == pid && ($6 == "libc" || $6 ~ /^AndroidRuntime:?$/) { print }
    ' "$log_file"
}

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
    local launcher
    launcher=$(resolve_launcher)
    msg "Launcher: $launcher"
    local attempt_dir since pid="" exit_info="" launch_valid=false
    local filters
    IFS=',' read -r -a filters <<< "$LOGCAT_FILTERS"
    for i in $(seq 1 $LAUNCH_RETRIES); do
        attempt_dir="$RESULTS_DIR/$(basename "$tests")/attempt-$i"
        mkdir -p "$attempt_dir" || return 1

        adb shell am force-stop "$PACKAGE" || return 1

        since=$(adb shell "date '+%Y-%m-%d %H:%M:%S.%3N'" | tr -d '\r')
        if [[ ! "$since" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}\ [0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3}$ ]]; then
            abort "Cannot establish device-clock launch boundary" "$EXIT_UNIT_TEST_RUN_CRASHED"
        fi

        # Start collecting logs
        adb logcat -b main -b system -v threadtime -T "${since:5}" -s "${filters[@]}" ActivityManager:I > "$attempt_dir/app.log" &
        LOGCAT_PIDS=("$!")
        adb logcat -b crash -v threadtime -T "${since:5}" > "$attempt_dir/crash.log" &
        LOGCAT_PIDS+=("$!")

        local deadline=$((SECONDS + TEST_TIMEOUT))
        launch_output=$(adb shell am start -W -n $launcher --es arg0 run-tests --es arg1 "$tests" 2>&1)
        launch_err=$?
        printf "%s\n" "$launch_output"

        if [[ $launch_err -ne 0 ]]; then
            error "Launch attempt failed with code: $launch_err"
        elif [[ "$launch_output" != *"LaunchState: COLD"* ]]; then
            error "Expected COLD launch but got different launch state from output"
        elif [[ "$launch_output" != *"Activity: $launcher"* && "$launch_output" != *"Activity: $PACKAGE/$MAIN_ACTIVITY"* ]]; then
            error "Expected activity '$launcher' or '$PACKAGE/$MAIN_ACTIVITY' but got different activity from launch output"
        else
            # Success
            launch_valid=true
        fi

        # Observe startup too: a crash can occur before pidof sees the process.
        local retry
        for retry in $(seq 1 $PID_RETRIES); do
            pid=$(find_pid_in_logs "$attempt_dir/app.log" "$attempt_dir/crash.log")
            if [[ "$pid" =~ ^[0-9]+$ ]]; then
                break
            fi

            adb shell dumpsys activity exit-info "$PACKAGE" > "$attempt_dir/exit-history.txt"
            exit_info=$(find_exit_info "$attempt_dir/exit-history.txt" "$since" "")
            if [[ "$exit_info" =~ pid=([0-9]+) ]]; then
                pid=${BASH_REMATCH[1]}
                break
            fi
            sleep 1
        done

        # Do not retry a started process - could hide a crash.
        if [ -n "$pid" ] || [ "$launch_valid" = true ] || [[ "$launch_output" == *"Status: ok"* ]]; then
            break
        fi

        stop_logcat
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

    echo "PID: $pid"
    msg "Evidence: $attempt_dir"

    # Stream app logs and monitor process termination.
    highlight "Reading logs..."
    if [ -n "$pid" ]; then
        tail -n +1 -f --pid="${LOGCAT_PIDS[0]}" "$attempt_dir/app.log" |
            awk -v pid="$pid" '$3 == pid { print; fflush() }' &
        LOG_STREAM_PID=$!
    fi

    local timed_out=false current_pid
    while [ -n "$pid" ]; do
        current_pid=$(adb shell pidof "$PACKAGE" 2>/dev/null | tr -d '\r')
        [[ " $current_pid " == *" $pid "* ]] || break
        if [ "$SECONDS" -ge "$deadline" ]; then
            timed_out=true
            break
        fi
        sleep 1
    done

    exit_info=""
    if [ "$timed_out" = false ]; then
        for retry in $(seq 1 $EXIT_INFO_RETRIES); do
            adb shell dumpsys activity exit-info "$PACKAGE" > "$attempt_dir/exit-history.txt"
            exit_info=$(find_exit_info "$attempt_dir/exit-history.txt" "$since" "$pid")
            [ -n "$exit_info" ] && break
            sleep 1
        done
    fi
    sleep "$CRASH_DRAIN_SECONDS"
    stop_logcat

    local exit_code=1  # Default to general failure
    local terminating=false
    local completed=false line
    while [ -n "$pid" ] && IFS= read -r line; do
        [[ "$line" =~ ^[^[:space:]]+[[:space:]]+[^[:space:]]+[[:space:]]+$pid[[:space:]] ]] || continue
        case "$line" in
            # Check for test run completion
            *">>> Test run complete with code:"*)
                if [[ "$line" =~ code:\ ([0-9]{1,3})[[:space:]]*$ ]] && [ "$((10#${BASH_REMATCH[1]}))" -le 255 ]; then
                    exit_code=$((10#${BASH_REMATCH[1]}))
                    completed=true
                fi
                ;;
            # Check Godot exit condition
            *"OnGodotTerminating"*)
                terminating=true
                ;;
        esac
    done < "$attempt_dir/app.log"
    filter_crash_log "$attempt_dir/crash.log" "$pid" > "$attempt_dir/crash-matched.log"

    local reason=unknown status=unknown signal_name=none reason_name="unverified exit (missing or ambiguous record)"
    if [[ "$exit_info" =~ reason=([0-9]+)\ (.+)\ subreason=.*\ status=([0-9]+)($|[[:space:]]) ]]; then
        reason=${BASH_REMATCH[1]}
        reason_name=${BASH_REMATCH[2]}
        status=${BASH_REMATCH[3]}
        if [[ "$reason" == 2 || "$reason" == 5 ]] && [ "$status" -gt 0 ]; then
            signal_name=$(kill -l "$status" 2>/dev/null) || signal_name=UNKNOWN
            signal_name="SIG$signal_name"
        fi
    fi

    # Godot normally quits on Android by killing its own process.
    if [ "$timed_out" = true ]; then
        exit_code=$EXIT_PROCESS_STILL_RUNNING_AFTER_TESTS
        error "Godot app process still running: runner_exit=$exit_code pid=$pid"
        adb shell am force-stop $PACKAGE
        adb shell pm clear "$PACKAGE" 2>/dev/null
    elif [[ "$launch_valid" != true || "$completed" != true || "$terminating" != true ||
            ( "$reason:$status" != 1:0 && "$reason:$status" != 2:9 ) ]] ||
            grep -Eq 'Fatal signal|FATAL EXCEPTION|signal [0-9]+ \(SIG' "$attempt_dir/crash-matched.log"; then
        # Acceptable exit reasons:
        # - reason=1, status=0: self exit, with code 0 (doesnt happen, but still valid).
        # - reason=2, status=9: signaled, SIGKILL - how we expect Godot to exit.
        # Other combinations take this path.
        exit_code=$EXIT_UNIT_TEST_RUN_CRASHED
        error "Unit test run crashed or exited uncleanly: runner_exit=$exit_code pid=${pid:-unknown} reason=$reason $reason_name status=$status signal=$signal_name"
        msg "Test completion: $completed; OnGodotTerminating: $terminating"
        printf '%s\n' "$exit_info"
        if [ -s "$attempt_dir/crash-matched.log" ]; then
            warning "$(cat "$attempt_dir/crash-matched.log")"
        else
            warning "No matching crash backtrace available in adb logcat -b crash."
        fi
    fi

    # Discard evidence after verified success.
    if [ "$exit_code" -eq 0 ]; then
        rm -rf -- "$attempt_dir"
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
        FAILED_TESTS+=("$godot_path ($(exit_code_reason "$test_exit_code"); exit code $test_exit_code)")
        error "✗ FAILED: $godot_path (exit code: $test_exit_code)"
        if [ "$test_exit_code" -eq "$EXIT_UNIT_TEST_RUN_CRASHED" ]; then
            OVERALL_EXIT_CODE=$EXIT_UNIT_TEST_RUN_CRASHED
        elif [[ "$test_exit_code" -eq "$EXIT_PROCESS_STILL_RUNNING_AFTER_TESTS" && "$OVERALL_EXIT_CODE" -ne "$EXIT_UNIT_TEST_RUN_CRASHED" ]]; then
            OVERALL_EXIT_CODE=$EXIT_PROCESS_STILL_RUNNING_AFTER_TESTS
        elif [ "$OVERALL_EXIT_CODE" -eq 0 ]; then
            OVERALL_EXIT_CODE=100
        fi
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
