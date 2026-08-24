#!/usr/bin/env pwsh

param (
    [ValidateSet("GDScript", "Dotnet", "All")]
    [string]$Suite = "GDScript",
    [string]$DeviceSerial = $env:SENTRY_TEST_DEVICE,
    [switch]$Help
)

Set-StrictMode -Version latest
$ErrorActionPreference = "Stop"

function Show-Usage {
    Write-Host @"
Usage: run-android-integration-tests.ps1 [-Suite GDScript|Dotnet|All] [-DeviceSerial <serial>] [-Help]

Export and run the integration tests on a local Android device available to ADB.

OPTIONS:
  -Suite           Test suite to run. Defaults to GDScript.
  -DeviceSerial    ADB serial to use. Required when multiple devices are online.
  -Help            Display this help message.

ENVIRONMENT VARIABLES:
  SENTRY_AUTH_TOKEN    Sentry API token used to retrieve test events (required)
  SENTRY_TEST_DSN      Test project DSN (defaults to the value in project.godot)
  SENTRY_TEST_DEVICE   ADB serial to use (same as -DeviceSerial)
  GODOT               Path to Godot (standard or .NET) for GDScript and All
  GODOT_DOTNET        Path to Godot .NET for Dotnet and All
"@
}

function Stop-WithError {
    param (
        [Parameter(Mandatory=$true)]
        [string]$Message
    )

    Write-Host $Message -ForegroundColor Red
    exit 1
}

function Get-AdbDevices {
    param (
        [Parameter(Mandatory=$true)]
        [string]$AdbPath
    )

    $output = & $AdbPath devices
    if ($LASTEXITCODE -ne 0) {
        Stop-WithError "Failed to list Android devices with ADB."
    }

    return @($output | ForEach-Object {
        if ($_ -match '^(\S+)\s+(\S+)') {
            [PSCustomObject]@{
                Serial = $matches[1]
                State = $matches[2]
            }
        }
    })
}

function Select-AdbDevice {
    param (
        [Parameter(Mandatory=$true)]
        [AllowEmptyCollection()]
        [object[]]$Devices,
        [string]$RequestedSerial
    )

    if (-not [string]::IsNullOrEmpty($RequestedSerial)) {
        $selectedDevice = $Devices | Where-Object { $_.Serial -eq $RequestedSerial } | Select-Object -First 1
        if ($null -eq $selectedDevice) {
            Stop-WithError "ADB device '$RequestedSerial' was not found."
        }
        if ($selectedDevice.State -ne "device") {
            Stop-WithError "ADB device '$RequestedSerial' is $($selectedDevice.State). Authorize it and try again."
        }
        return $selectedDevice
    }

    $onlineDevices = @($Devices | Where-Object { $_.State -eq "device" })
    if ($onlineDevices.Count -eq 0) {
        $knownDevices = ($Devices | ForEach-Object { "$($_.Serial) ($($_.State))" }) -join ", "
        if ([string]::IsNullOrEmpty($knownDevices)) {
            $knownDevices = "none"
        }
        Stop-WithError "No authorized Android device is online. ADB devices: $knownDevices."
    }
    if ($onlineDevices.Count -gt 1) {
        $serials = ($onlineDevices.Serial | Sort-Object) -join ", "
        Stop-WithError "Multiple Android devices are online ($serials). Select one with -DeviceSerial."
    }

    return $onlineDevices[0]
}

function Resolve-GodotExecutable {
    param (
        [Parameter(Mandatory=$true)]
        [string]$Executable,
        [switch]$RequireDotnet
    )

    $command = Get-Command $Executable -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        Stop-WithError "Godot executable '$Executable' was not found."
    }

    $version = & $command.Source --version
    if ($LASTEXITCODE -ne 0) {
        Stop-WithError "Could not determine the Godot version for '$Executable'."
    }

    $isDotnet = $version -match '\.mono\.'
    if ($RequireDotnet -and -not $isDotnet) {
        Stop-WithError "Godot executable '$Executable' is not a .NET build."
    }

    return [PSCustomObject]@{
        Path = $command.Source
        IsDotnet = $isDotnet
    }
}

function Test-ApkContainsMonoRuntime {
    param (
        [Parameter(Mandatory=$true)]
        [string]$ApkPath
    )

    $archive = [System.IO.Compression.ZipFile]::OpenRead($ApkPath)
    try {
        return $null -ne ($archive.Entries | Where-Object {
            $_.FullName -match '(^|/)libmonosgen-2\.0\.so$'
        } | Select-Object -First 1)
    }
    finally {
        $archive.Dispose()
    }
}

if ($Help) {
    Show-Usage
    exit 0
}

if ([string]::IsNullOrEmpty($env:SENTRY_AUTH_TOKEN)) {
    Stop-WithError "SENTRY_AUTH_TOKEN is not set."
}

$requiresGdScript = $Suite -in @("GDScript", "All")
$requiresDotnet = $Suite -in @("Dotnet", "All")

$adbCommand = Get-Command "adb" -ErrorAction SilentlyContinue
if ($null -eq $adbCommand) {
    Stop-WithError "ADB was not found in PATH."
}

$godotInfo = $null
if ($requiresGdScript) {
    $godot = if ([string]::IsNullOrEmpty($env:GODOT)) { "godot" } else { $env:GODOT }
    $godotInfo = Resolve-GodotExecutable -Executable $godot
}

$godotDotnetInfo = $null
if ($requiresDotnet) {
    $godotDotnet = if (-not [string]::IsNullOrEmpty($env:GODOT_DOTNET)) {
        $env:GODOT_DOTNET
    } elseif ($Suite -eq "Dotnet" -and -not [string]::IsNullOrEmpty($env:GODOT)) {
        $env:GODOT
    } else {
        "godot"
    }
    $godotDotnetInfo = Resolve-GodotExecutable -Executable $godotDotnet -RequireDotnet
}

$requiresDotnetBuild = $requiresDotnet -or ($requiresGdScript -and $godotInfo.IsDotnet)

$pesterModule = Get-Module -ListAvailable "Pester" | Sort-Object Version -Descending | Select-Object -First 1
if ($null -eq $pesterModule) {
    Stop-WithError "Pester is not installed. Run: Install-Module -Name Pester -Force -SkipPublisherCheck"
}

$dotnetCommand = $null
if ($requiresDotnetBuild) {
    $dotnetCommand = Get-Command "dotnet" -ErrorAction SilentlyContinue
    if ($null -eq $dotnetCommand) {
        Stop-WithError "The dotnet CLI was not found in PATH."
    }
}

$adbDevices = @(Get-AdbDevices -AdbPath $adbCommand.Source)
$selectedDevice = Select-AdbDevice -Devices $adbDevices -RequestedSerial $DeviceSerial
Write-Host "Using Android device: $($selectedDevice.Serial)" -ForegroundColor Cyan

$repoRoot = Split-Path -Parent $PSScriptRoot
$projectDir = Join-Path $repoRoot "project"
$exportDir = Join-Path $repoRoot "exports"
$apkPath = Join-Path $exportDir "android.apk"
$integrationDir = Join-Path $repoRoot "tests/integration"
$testRuns = @()
if ($requiresGdScript) {
    $testRuns += [PSCustomObject]@{
        Name = "GDScript"
        BuildDotnet = $godotInfo.IsDotnet
        GodotExecutable = $godotInfo.Path
        TestScript = Join-Path $integrationDir "Integration.Tests.ps1"
    }
}
if ($requiresDotnet) {
    $testRuns += [PSCustomObject]@{
        Name = "Dotnet"
        BuildDotnet = $true
        GodotExecutable = $godotDotnetInfo.Path
        TestScript = Join-Path $integrationDir "Integration.Dotnet.Tests.ps1"
    }
}

# Snapshot CSharp project to restore it later.
$dotnetProject = Join-Path $projectDir "Sentry demo project.csproj"
$dotnetProjectOldPath = "$dotnetProject.old"
$dotnetProjectContents = [System.IO.File]::ReadAllBytes($dotnetProject)
$dotnetProjectOldExisted = Test-Path $dotnetProjectOldPath -PathType Leaf
$dotnetProjectOldContents = $null
if ($dotnetProjectOldExisted) {
    $dotnetProjectOldContents = [System.IO.File]::ReadAllBytes($dotnetProjectOldPath)
}

$results = @()
try {
    Import-Module $pesterModule.Path
    foreach ($testRun in $testRuns) {
        if ($testRun.BuildDotnet) {
            Write-Host "Building .NET project..." -ForegroundColor Cyan
            & $dotnetCommand.Source build $dotnetProject --nologo
            if ($LASTEXITCODE -ne 0) {
                Stop-WithError ".NET project build failed (exit code $LASTEXITCODE)."
            }
        }

        New-Item -ItemType Directory -Path $exportDir -Force | Out-Null
        Remove-Item -Path $apkPath -Force -ErrorAction SilentlyContinue

        Write-Host "Exporting Android test APK for $($testRun.Name)..." -ForegroundColor Cyan
        & $testRun.GodotExecutable `
            --headless `
            --disable-crash-handler `
            --path $projectDir `
            --install-android-build-template `
            --export-debug "Android Tests" `
            $apkPath
        $exportExitCode = $LASTEXITCODE

        if ($exportExitCode -ne 0) {
            Write-Warning "Godot returned exit code $exportExitCode while exporting. Checking the APK before continuing."
        }
        if (-not (Test-Path $apkPath -PathType Leaf)) {
            Stop-WithError "Android test APK was not created at '$apkPath' (export exit code $exportExitCode). Check the Android export templates and SDK configuration."
        }
        if ($testRun.BuildDotnet -and -not (Test-ApkContainsMonoRuntime -ApkPath $apkPath)) {
            Stop-WithError "The Android test APK does not contain the Mono runtime. Check that matching Godot .NET Android export templates are installed."
        }

        $env:SENTRY_TEST_PLATFORM = "Adb"
        $env:SENTRY_TEST_DEVICE = $selectedDevice.Serial
        $env:SENTRY_TEST_EXECUTABLE = $apkPath

        $results += Invoke-Pester -Path $testRun.TestScript -PassThru
    }
}
finally {
    [System.IO.File]::WriteAllBytes($dotnetProject, $dotnetProjectContents)
    if ($dotnetProjectOldExisted) {
        [System.IO.File]::WriteAllBytes($dotnetProjectOldPath, $dotnetProjectOldContents)
    } else {
        Remove-Item -Path $dotnetProjectOldPath -Force -ErrorAction SilentlyContinue
    }
}

if ($results.Result -contains "Failed") {
    exit 1
}
