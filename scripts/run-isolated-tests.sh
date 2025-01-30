#!/bin/sh
# Run tests that require isolation.
# Such tests are located in the "project/test/isolated" directory.

highlight() {
    echo -e "\033[1;36m$*\033[0m"
}

if [[ -z "$GODOT" ]]; then
    echo "GODOT environment variable is not set. Defaulting to \"godot\"."
    GODOT=godot
fi

start_dir=$(pwd)
script_dir=$(dirname $(readlink -f "$0"))
cd "$script_dir/../project"

exit_code=0
num_failed=0
num_passed=0

for file in test/isolated/test_*; do
    highlight "Running isolated test: $file"
    ${GODOT} --headless --path . -s "res://addons/gdUnit4/bin/GdUnitCmdTool.gd" --ignoreHeadlessMode -c -a "$file"

    err=$?
    if [ $err -ne 0 ]; then
        exit_code=$err
        ((num_failed++))
    else
        ((num_passed++))
    fi
done

cd "$start_dir"

echo "--------------------------------------------------------------------------------"
highlight "Tests finished."
echo "Summary: $num_passed passed, $num_failed failed."
if [ $exit_code -eq 0 ]; then
    echo "All isolated test suites passed."
else
    echo "WARNING: Some isolated test suites failed!" >&2
fi
exit $exit_code
