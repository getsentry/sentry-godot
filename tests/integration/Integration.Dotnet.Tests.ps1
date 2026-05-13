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

$DotnetCommonTestCases = @(
    @{ Name = "Exits with code zero"; TestBlock = {
            param($TestSetup, $RunResult)
            if ($TestSetup.IsAndroid) {
                # app-runner doesn't support exit code on Android.
                return
            }
            $RunResult.ExitCode | Should -Be 0
        }
    }
    @{ Name = "Native layer is enabled"; TestBlock = {
            param($RunResult)
            # On Android each line carries a logcat prefix, so match the marker anywhere in the line.
            $line = $RunResult.Output | Where-Object {
                $_ -match "SENTRY_NATIVE_ENABLED: (true|false)"
            } | Select-Object -First 1
            $line | Should -Match "SENTRY_NATIVE_ENABLED: true"
        }
    }
    @{ Name = ".NET layer is enabled"; TestBlock = {
            param($RunResult)
            $line = $RunResult.Output | Where-Object {
                $_ -match "SENTRY_DOTNET_ENABLED: (true|false)"
            } | Select-Object -First 1
            $line | Should -Match "SENTRY_DOTNET_ENABLED: true"
        }
    }
    @{ Name = "Has correct exception type"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.exception.values[0].type | Should -Be "System.Exception"
        }
    }
    @{ Name = "Has correct exception value"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.exception.values[0].value | Should -Be "Exception (should be captured)"
        }
    }
    @{ Name = "Has Godot.Bridge mechanism"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.exception.values[0].mechanism.type | Should -Be "Godot.Bridge"
            $SentryEvent.exception.values[0].mechanism.handled | Should -Be $false
        }
    }
    @{ Name = "Has stacktrace frames"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.exception.values[0].stacktrace.frames | Should -Not -BeNullOrEmpty
        }
    }
)

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

            $script:dotnetInitRunResult      = Invoke-TestAction -Action "dotnet-exception-capture" -AdditionalArgs @("plain")
            $script:bareRethrowRunResult    = Invoke-TestAction -Action "dotnet-exception-capture" -AdditionalArgs @("bare-rethrow")
            $script:wrappedRethrowRunResult = Invoke-TestAction -Action "dotnet-exception-capture" -AdditionalArgs @("wrapped-rethrow")
            $script:gdscriptInitRunResult   = Invoke-TestAction -Action "dotnet-capture-via-gdscript-init"
            $script:crossLayerRunResult     = Invoke-TestAction -Action "dotnet-cross-layer-capture"

            # Auto-init reads options from project settings. Use override.cfg to enable it just for this run.
            # Mobile exports are sealed, so override.cfg cannot be applied there.
            if ($env:SENTRY_TEST_PLATFORM -notin @("Adb", "AndroidSauceLabs", "iOSSauceLabs")) {
                $overridePath = Join-Path $PSScriptRoot "../../project/override.cfg"
                try {
                    # Keep release/environment/dist aligned with cli_commands.gd, because auto-init runs without an init
                    # callback, but the tests still expect these values.
                    Set-Content -Path $overridePath -Value @"
[sentry]

options/auto_init=true
options/release="test-app@1.0.0"
options/environment="integration-test"
options/dist="test-dist"
options/debug_printing=0
"@
                    $script:autoInitRunResult = Invoke-TestAction -Action "dotnet-capture-via-auto-init"
                } finally {
                    Remove-Item $overridePath -ErrorAction SilentlyContinue
                }
            }
        }
        finally {
            Disconnect-Device
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

    Context "Dotnet with CSharp driving init" {
        BeforeAll {
            $runResult = $script:dotnetInitRunResult

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

        It "<Name>" -ForEach $DotnetCommonTestCases {
            & $TestBlock -SentryEvent $runEvent -RunResult $runResult -TestSetup $script:TestSetup
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

        It "<Name>" -ForEach $DotnetCommonTestCases {
            & $TestBlock -SentryEvent $runEvent -RunResult $runResult -TestSetup $script:TestSetup
        }
    }

    Context "Dotnet with native auto-init driving init" -Skip:($env:SENTRY_TEST_PLATFORM -in @("Adb", "AndroidSauceLabs", "iOSSauceLabs")) {
        BeforeAll {
            $runResult = $script:autoInitRunResult

            $eventId = Get-EventIds -AppOutput $runResult.Output -ExpectedCount 1
            if ($eventId) {
                Write-GitHub "::group::Getting event content"
                $script:runEvent = Get-SentryTestEvent -EventId "$eventId"
                Write-GitHub "::endgroup::"
            }
        }

        It "<Name>" -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType "dotnet-capture-via-auto-init" -RunResult $runResult -TestSetup $script:TestSetup
        }

        It "<Name>" -ForEach $DotnetCommonTestCases {
            & $TestBlock -SentryEvent $runEvent -RunResult $runResult -TestSetup $script:TestSetup
        }
    }

    Context "Cross-layer capture" {
        BeforeAll {
            $runResult = $script:crossLayerRunResult

            $eventIds = Get-EventIds -AppOutput $runResult.Output -ExpectedCount 2
            Write-GitHub "::group::Getting event content"
            $script:nativeEvent  = Get-SentryTestEvent -EventId $eventIds[0]
            $script:managedEvent = Get-SentryTestEvent -EventId $eventIds[1]
            Write-GitHub "::endgroup::"
        }

        It "Exits with code zero" {
            if ($TestSetup.IsAndroid) {
                # app-runner doesn't support exit code on Android.
                return
            }
            $runResult.ExitCode | Should -Be 0
        }

        It "Native event has trace context with non-empty trace_id" {
            $nativeEvent.contexts.trace | Should -Not -BeNullOrEmpty
            $nativeEvent.contexts.trace.trace_id | Should -Not -BeNullOrEmpty
        }

        It "Managed event has trace context with non-empty trace_id" {
            $managedEvent.contexts.trace | Should -Not -BeNullOrEmpty
            $managedEvent.contexts.trace.trace_id | Should -Not -BeNullOrEmpty
        }

        It "Native and managed events share the same trace_id" {
            $nativeEvent.contexts.trace.trace_id | Should -Be $managedEvent.contexts.trace.trace_id
        }

        It "Native event contains tag set from .NET" {
            ($nativeEvent.tags | Where-Object { $_.key -eq "dotnet.scope.synced" }).value | Should -Be "from-dotnet"
        }

        It "Native event omits tag removed from .NET" {
            ($nativeEvent.tags | Where-Object { $_.key -eq "dotnet.scope.removed" }) | Should -BeNullOrEmpty
        }

        It "Native event includes breadcrumb added from .NET" {
            $nativeEvent.breadcrumbs.values | Where-Object { $_.message -eq "Synced from .NET" } | Should -Not -BeNullOrEmpty
        }

        It "Native event contains user context set from .NET" {
            $nativeEvent.user | Should -Not -BeNullOrEmpty
            $nativeEvent.user.id | Should -Be "99999"
            $nativeEvent.user.username | Should -Be "DotnetSyncedUser"
            $nativeEvent.user.email | Should -Be "dotnet-synced@test.abc"
            $nativeEvent.user.ip_address | Should -Be "1.2.3.4"
        }

        It "Native event includes breadcrumb data marshalled from .NET" {
            $crumbs = @($nativeEvent.breadcrumbs.values | Where-Object { $_.message -eq "Synced data breadcrumb from .NET" })
            $crumbs | Should -HaveCount 1
            $crumb = $crumbs[0]
            $crumb.category | Should -Be "dotnet.probe"
            $crumb.type | Should -Be "http"
            $crumb.data | Should -Not -BeNullOrEmpty
            $crumb.data."http.url" | Should -Be "https://example.test/api/v1/probe"
            $crumb.data."http.status" | Should -Be "200"
            $crumb.data."unicode" | Should -Be "Hello 世界! 👋"
        }

        It "Managed event omits tag set in per-call scope callback" {
            # Sanity check: upstream CaptureEvent clones the current scope and discards it.
            ($managedEvent.tags | Where-Object { $_.key -eq "dotnet.per_call_scope.tag" }) | Should -BeNullOrEmpty
        }

        It "Native event omits tag set in per-call scope callback" {
            ($nativeEvent.tags | Where-Object { $_.key -eq "dotnet.per_call_scope.tag" }) | Should -BeNullOrEmpty
        }

        It "Native event omits breadcrumb added in per-call scope callback" {
            $nativeEvent.breadcrumbs.values | Where-Object { $_.message -eq "Per-call leak breadcrumb" } | Should -BeNullOrEmpty
        }

        It "Native event preserves tag despite UnsetTag in per-call scope callback" {
            ($nativeEvent.tags | Where-Object { $_.key -eq "dotnet.scope.synced" }).value | Should -Be "from-dotnet"
        }

        It "Native event preserves user despite SetUser in per-call scope callback" {
            $nativeEvent.user.id | Should -Be "99999"
            $nativeEvent.user.username | Should -Be "DotnetSyncedUser"
            $nativeEvent.user.email | Should -Be "dotnet-synced@test.abc"
        }
    }
}
