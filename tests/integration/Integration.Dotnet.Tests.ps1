#!/usr/bin/env pwsh
#
# .NET integration tests for Sentry Godot SDK
#
# Environment variables:
#   SENTRY_TEST_EXECUTABLE: command line to run the project
#   SENTRY_TEST_ARGS: arguments
#   SENTRY_TEST_DSN: test DSN
#   SENTRY_AUTH_TOKEN: authentication token for Sentry API
#   SENTRY_TEST_PLATFORM: test platform (aka device provider) such as "Local"
#   GODOT_DOTNET: path to a Godot mono binary, used when SENTRY_TEST_EXECUTABLE is unset

Set-StrictMode -Version latest
$ErrorActionPreference = "Stop"
$global:DebugPreference = "Continue"

. $PSScriptRoot/../../modules/app-runner/import-modules.ps1
. $PSScriptRoot/CommonTestCases.ps1
. $PSScriptRoot/Utils.ps1

BeforeAll {
    function Invoke-TestAction {
        param (
            [Parameter(Mandatory=$true)]
            [string]$Action,
            [string[]]$AdditionalArgs = @()
        )

        Write-Host "Running $Action..."

        $args = $script:TestSetup.Args + @($Action) + $AdditionalArgs
        $execPath = $script:TestSetup.Executable

        if ($script:TestSetup.IsAndroid) {
            $args = ConvertTo-AndroidExtras -Arguments $args
            $execPath = $script:TestSetup.AndroidComponent
        } elseif ($script:TestSetup.Platform -match "iOS") {
            $execPath = $script:TestSetup.iOSBundleId
        }

        $logFilePath = if ($script:TestSetup.Platform -eq "iOSSauceLabs") { $script:TestSetup.iOSApplicationLogFile } else { $null }
        $runResult = Invoke-DeviceApp -ExecutablePath $execPath -Arguments $args -LogFilePath $logFilePath

        $runResult | ConvertTo-Json -Depth 5 | Out-File -FilePath (Get-OutputFilePath "${Action}-result.json")

        return $runResult
    }

    New-Item -ItemType Directory -Path "$PSScriptRoot/results/" -ErrorAction Continue 2>&1 | Out-Null
    Set-OutputDir -Path "$PSScriptRoot/results/"

    $script:TestSetup = [PSCustomObject]@{
        Executable = $env:SENTRY_TEST_EXECUTABLE
        Args = @()
        Dsn = $env:SENTRY_TEST_DSN
        AuthToken = $env:SENTRY_AUTH_TOKEN
        Platform = $env:SENTRY_TEST_PLATFORM
        AndroidComponent = "io.sentry.godot.project/com.godot.game.GodotApp"
        IsAndroid = ($env:SENTRY_TEST_PLATFORM -in @("Adb", "AndroidSauceLabs"))
        IsCocoa = ($env:SENTRY_TEST_PLATFORM -ieq "macOS" -or $env:SENTRY_TEST_PLATFORM -match "iOS" -or
            (($env:SENTRY_TEST_PLATFORM -ieq "Local" -or [string]::IsNullOrEmpty($env:SENTRY_TEST_PLATFORM)) -and $IsMacOS))
        IsWeb = $false
        IsDotnet = $true
        iOSBundleId = "io.sentry.SentryGodotProject"
        iOSApplicationLogFile = "@io.sentry.SentryGodotProject:documents/logs/godot.log"
    }

    if ([string]::IsNullOrEmpty($script:TestSetup.Executable)) {
        if (-not [string]::IsNullOrEmpty($env:GODOT_DOTNET)) {
            Write-Warning "SENTRY_TEST_EXECUTABLE environment variable is not set. Defaulting to env:GODOT_DOTNET."
            $script:TestSetup.Executable = $env:GODOT_DOTNET
        } else {
            Write-Warning "SENTRY_TEST_EXECUTABLE and GODOT_DOTNET environment variables are not set. Defaulting to env:GODOT."
            $script:TestSetup.Executable = $env:GODOT
        }
        $script:TestSetup.Args += @("--disable-crash-handler", "--headless", "--path", "project", "--")
    } else {
        $script:TestSetup.Args += @("--")
    }
    if (-not (Test-Path $script:TestSetup.Executable)) {
        throw "Executable not found at: $($script:TestSetup.Executable)"
    }

    if ([string]::IsNullOrEmpty($script:TestSetup.Dsn)) {
        $projectGodotPath = Join-Path $PSScriptRoot "../../project/project.godot"
        if (Test-Path $projectGodotPath) {
            Write-Warning "SENTRY_TEST_DSN environment variable is not set. Reading DSN from project.godot..."
            $projectContent = Get-Content $projectGodotPath -Raw
            if ($projectContent -match 'options/dsn="([^"]+)"') {
                $script:TestSetup.Dsn = $matches[1]
            } else {
                throw "Could not find DSN in project.godot file"
            }
        } else {
            throw "Could not find project.godot file at $projectGodotPath"
        }
    }

    if ([string]::IsNullOrEmpty($script:TestSetup.AuthToken)) {
        throw "SENTRY_AUTH_TOKEN environment variable is not set."
    }

    if ([string]::IsNullOrEmpty($script:TestSetup.Platform)) {
        Write-Warning "SENTRY_TEST_PLATFORM environment variable is not set. Defaulting to 'Local'."
        $script:TestSetup.Platform = "Local"
    }

    Connect-SentryApi `
        -ApiToken $script:TestSetup.AuthToken `
        -DSN $script:TestSetup.Dsn
}


AfterAll {
    Disconnect-SentryApi
}


Describe ".NET Integration Tests" {
    BeforeAll {
        try {
            Connect-Device -Platform $script:TestSetup.Platform
            Install-DeviceApp -Path $script:TestSetup.Executable

            $script:exceptionRunResult      = Invoke-TestAction -Action "dotnet-exception-capture" -AdditionalArgs @("plain")
            $script:bareRethrowRunResult    = Invoke-TestAction -Action "dotnet-exception-capture" -AdditionalArgs @("bare-rethrow")
            $script:wrappedRethrowRunResult = Invoke-TestAction -Action "dotnet-exception-capture" -AdditionalArgs @("wrapped-rethrow")
            $script:gdscriptInitRunResult   = Invoke-TestAction -Action "dotnet-capture-via-gdscript-init"
        }
        finally {
            Disconnect-Device
        }
    }

    Context "Plain Exception" {
        BeforeAll {
            $runResult = $script:exceptionRunResult

            $eventId = Get-EventIds -AppOutput $runResult.Output -ExpectedCount 1
            if ($eventId) {
                Write-GitHub "::group::Getting event content"
                $script:runEvent = Get-SentryTestEvent -EventId "$eventId"
                Write-GitHub "::endgroup::"
            }
        }

        It "<Name>" -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType "dotnet-exception-capture-plain" -RunResult $runResult -TestSetup $script:TestSetup
        }

        It "Exits with code zero" {
            if ($TestSetup.IsAndroid) {
                # app-runner doesn't support exit code on Android.
                return
            }
            $runResult.ExitCode | Should -Be 0
        }

        It "Has correct exception type" {
            $runEvent.exception.values[0].type | Should -Be "System.Exception"
        }

        It "Has correct exception value" {
            $runEvent.exception.values[0].value | Should -Be "Exception (should be captured)"
        }

        It "Has Godot.Bridge mechanism" {
            $runEvent.exception.values[0].mechanism.type | Should -Be "Godot.Bridge"
            $runEvent.exception.values[0].mechanism.handled | Should -Be $false
        }

        It "Has stacktrace frames" {
            $runEvent.exception.values[0].stacktrace.frames | Should -Not -BeNullOrEmpty
        }
    }

    Context "Bare Rethrow" {
        BeforeAll {
            $runResult = $script:bareRethrowRunResult

            $eventId = Get-EventIds -AppOutput $runResult.Output -ExpectedCount 1
            if ($eventId) {
                Write-GitHub "::group::Getting event content"
                $script:runEvent = Get-SentryTestEvent -EventId "$eventId"
                Write-GitHub "::endgroup::"
            }
        }

        It "<Name>" -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType "dotnet-exception-capture-bare-rethrow" -RunResult $runResult -TestSetup $script:TestSetup
        }

        It "Exits with code zero" {
            if ($TestSetup.IsAndroid) {
                # app-runner doesn't support exit code on Android.
                return
            }
            $runResult.ExitCode | Should -Be 0
        }

        It "Has correct exception type" {
            $runEvent.exception.values[0].type | Should -Be "System.Exception"
        }

        It "Has correct exception value" {
            $runEvent.exception.values[0].value | Should -Be "Bare rethrow (should be captured)"
        }

        It "Has Godot.Bridge mechanism" {
            $runEvent.exception.values[0].mechanism.type | Should -Be "Godot.Bridge"
            $runEvent.exception.values[0].mechanism.handled | Should -Be $false
        }
    }

    Context "Wrapped Rethrow" {
        BeforeAll {
            $runResult = $script:wrappedRethrowRunResult

            $eventId = Get-EventIds -AppOutput $runResult.Output -ExpectedCount 1
            if ($eventId) {
                Write-GitHub "::group::Getting event content"
                $script:runEvent = Get-SentryTestEvent -EventId "$eventId"
                Write-GitHub "::endgroup::"
            }
        }

        It "<Name>" -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType "dotnet-exception-capture-wrapped-rethrow" -RunResult $runResult -TestSetup $script:TestSetup
        }

        It "Exits with code zero" {
            if ($TestSetup.IsAndroid) {
                # app-runner doesn't support exit code on Android.
                return
            }
            $runResult.ExitCode | Should -Be 0
        }

        It "Has wrapper as the outermost captured exception" {
            # exception.values is innermost-first per Sentry's protocol; the
            # wrapper is the outermost throw, so it lands last.
            $values = $runEvent.exception.values
            $values | Should -Not -BeNullOrEmpty
            $outermost = $values[$values.Count - 1]
            $outermost.type | Should -Be "System.InvalidOperationException"
            $outermost.value | Should -Be "Wrapped exception"
        }

        It "Includes the inner exception in the chain" {
            $values = $runEvent.exception.values
            $values.Count | Should -BeGreaterOrEqual 2
            $inner = $values[0]
            $inner.type | Should -Be "System.Exception"
            $inner.value | Should -Be "Inner exception"
        }

        It "Has Godot.Bridge mechanism on outermost exception" {
            $values = $runEvent.exception.values
            $outermost = $values[$values.Count - 1]
            $outermost.mechanism.type | Should -Be "Godot.Bridge"
            $outermost.mechanism.handled | Should -Be $false
        }
    }

    Context "Dotnet with GDScript driving init" {
        BeforeAll {
            $runResult = $script:gdscriptInitRunResult

            $eventId = Get-EventIds -AppOutput $runResult.Output -ExpectedCount 1
            if ($eventId) {
                Write-GitHub "::group::Getting event content"
                $script:runEvent = Get-SentryTestEvent -EventId "$eventId"
                Write-GitHub "::endgroup::"
            }
        }

        It "<Name>" -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType "dotnet-capture-via-gdscript-init" -RunResult $runResult -TestSetup $script:TestSetup
        }

        It "Exits with code zero" {
            if ($TestSetup.IsAndroid) {
                # app-runner doesn't support exit code on Android.
                return
            }
            $runResult.ExitCode | Should -Be 0
        }

        It "Has correct exception type" {
            $runEvent.exception.values[0].type | Should -Be "System.Exception"
        }

        It "Has correct exception value" {
            $runEvent.exception.values[0].value | Should -Be "Exception (should be captured)"
        }

        It "Has Godot.Bridge mechanism" {
            $runEvent.exception.values[0].mechanism.type | Should -Be "Godot.Bridge"
            $runEvent.exception.values[0].mechanism.handled | Should -Be $false
        }
    }
}
