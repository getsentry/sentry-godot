#!/usr/bin/env pwsh

# Run the test suites and the isolated tests on the local desktop platform.

param (
    [switch]$Help
)

# Configuration
$TestTimeout = 120  # seconds per test run

# Special exit codes
$ExitTestRunTimedOut = 89

function Show-Usage {
    Write-Host @"
Usage: run-desktop-tests.ps1 [-Help]

Run the test suites and the isolated tests on the local desktop platform.

Isolated tests need a clean SDK state, so every test path runs in its own Godot
process through the project's "run-tests" command, the same way the Android and
Web runners execute these tests.

ENVIRONMENT VARIABLES:
  GODOT    Path to the Godot executable (falls back to "godot" in PATH)
"@
}

# Formatted output
function Write-Highlight { param ([string]$Message) Write-Host $Message -ForegroundColor Cyan }
function Write-Msg       { param ([string]$Message) Write-Host $Message }
function Write-Err       { param ([string]$Message) Write-Host $Message -ForegroundColor Red }
function Write-Success   { param ([string]$Message) Write-Host $Message -ForegroundColor Green }
function Write-GitHub    { param ([string]$Message) if ($env:GITHUB_ACTIONS -eq "true") { Write-Host $Message } }

if ($Help) {
    Show-Usage
    exit 0
}

$godot = $env:GODOT
if (-not $godot) {
    Write-Msg "GODOT environment variable is not set. Defaulting to `"godot`"."
    $godot = "godot"
}

if (-not (Get-Command $godot -ErrorAction SilentlyContinue)) {
    Write-Err "Godot executable not found. Please set the GODOT environment variable. Aborting."
    exit 1
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$projectDir = Join-Path $repoRoot "project"
$isolatedDir = Join-Path $projectDir "test/isolated"

# Discover isolated test suites and add normal suites as first item
Write-Highlight "Looking for isolated test suites..."
$isolatedTests = @(Get-ChildItem -Path $isolatedDir -Filter "test_*.gd" | Sort-Object Name)

if ($isolatedTests.Count -eq 0) {
    Write-Err "No isolated test files found in $isolatedDir. Aborting."
    exit 1
}

Write-Msg "Found $($isolatedTests.Count) isolated test suites."

$testPaths = @("res://test/suites/") + ($isolatedTests | ForEach-Object { "res://test/isolated/$($_.Name)" })

$overallExitCode = 0
$passedTests = @()
$failedTests = @()

# For each test path perform a separate run
foreach ($testPath in $testPaths) {
    Write-Host ""
    Write-Highlight "Running tests: $testPath"
    Write-GitHub "::group::Test log $testPath"

    $godotArgs = @("--headless", "--path", $projectDir, "--", "run-tests", $testPath)
    $process = Start-Process $godot -ArgumentList $godotArgs -PassThru -NoNewWindow

    if ($process.WaitForExit($TestTimeout * 1000)) {
        $testExitCode = $process.ExitCode
    } else {
        # A hung run would otherwise block until the CI step timeout kills the whole job.
        Write-Err "Test run exceeded $TestTimeout seconds. Terminating process $($process.Id)."
        if (-not $process.HasExited) {
            $process.Kill()
        }
        $process.WaitForExit()
        $testExitCode = $ExitTestRunTimedOut
    }

    Write-GitHub "::endgroup::"

    if ($testExitCode -eq 0) {
        $passedTests += $testPath
        Write-Success "PASSED: $testPath"
    } else {
        $failedTests += $testPath
        Write-Err "FAILED: $testPath (exit code: $testExitCode)"
        $overallExitCode = 100
    }
}

# Summary
Write-Host ""
Write-Highlight "Final Results"
Write-Msg "Passed: $($passedTests.Count)"
Write-Msg "Failed: $($failedTests.Count)"

if ($failedTests.Count -gt 0) {
    Write-Err "Failed tests:"
    foreach ($failedTest in $failedTests) {
        Write-Err "  - $failedTest"
    }
}

Write-Highlight "Test execution finished with code: $overallExitCode"
exit $overallExitCode
