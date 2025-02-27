#!/usr/bin/env pwsh

# Run tests that require isolation.
# Such tests are located in the "project/test/isolated" directory.

function Highlight {
    param (
        [string]$Message
    )
    Write-Host $Message -ForegroundColor Cyan
}

$godot = $env:GODOT

if (-not $godot) {
    Write-Host "GODOT environment variable is not set. Defaulting to `"godot`"."
    $godot = "godot"
}

if (-not (Get-Command $godot -ErrorAction SilentlyContinue)) {
    Write-Error "Godot executable not found. Please set the GODOT environment variable." -CategoryActivity "ERROR"
    exit 1
}

$startDir = Get-Location
$scriptDir = Split-Path -Parent (Resolve-Path $MyInvocation.MyCommand.Path)
Set-Location "$scriptDir/../project"

$exitCode = 0
$numFailed = 0
$numPassed = 0

Get-ChildItem -Path "test/isolated" -Filter "test_*" | ForEach-Object {
    $file = $_.FullName
    Highlight "Running isolated test: $file"

    $args = "--headless --path . -s `"res://addons/gdUnit4/bin/GdUnitCmdTool.gd`" --ignoreHeadlessMode -c -a `"$file`""
    $process = Start-Process $godot -ArgumentList $args -PassThru -Wait -NoNewWindow
    $err = $process.ExitCode

    Highlight "Finished with exit code: $err" 

    if ($err -ne 0) {
        $exitCode = $err
        $numFailed++
    } else {
        $numPassed++
    }
}

Set-Location $startDir

Write-Host "--------------------------------------------------------------------------------"
Highlight "Tests finished."
Write-Host "Summary: $numPassed passed, $numFailed failed."
if ($exitCode -eq 0) {
    Write-Host "SUCCESS: All isolated tests suites passed." -ForegroundColor Green
} else {
    Write-Warning "Some isolated test suites failed!"
}

exit $exitCode
