#!/usr/bin/env pwsh
#
# Integration tests for Sentry Godot SDK
#
# Environment variables:
#   SENTRY_TEST_EXECUTABLE: command line to run the project
#   SENTRY_TEST_ARGS: arguments
#   SENTRY_TEST_DSN: test DSN
#   SENTRY_TEST_PLATFORM: test platform (aka device provider) such as "Local" or "Android"
#   SENTRY_AUTH_TOKEN: authentication token for Sentry API

Set-StrictMode -Version latest
$ErrorActionPreference = 'Stop'
$global:DebugPreference = 'Continue'

# Import app-runner modules
. $PSScriptRoot/../modules/app-runner/import-modules.ps1

BeforeAll {
    function Write-GitHub
    {
        param (
            [Parameter(Mandatory=$true)]
            [string]$message
        )
        if ($env:GITHUB_ACTIONS)
        {
            Write-Host "${message}"
        } else
        {
            Write-Host "GITHUB: ${message}"
        }
    }

    Write-Debug "BEFORE-ALL"

    # Create directory for the test results
    New-Item -ItemType Directory -Path "$PSScriptRoot/results/" 2>&1 | Out-Null
    Set-OutputDir -Path "$PSScriptRoot/results/"

    # Initialize test arguments
    $script:testArgs = $env:SENTRY_TEST_ARGS

    # Initialize executable
    $script:testExecutable = $env:SENTRY_TEST_EXECUTABLE
    if ([string]::IsNullOrEmpty($script:testExecutable))
    {
        Write-Warning "SENTRY_TEST_EXECUTABLE environment variable is not set. Defaulting to env:GODOT."
        $script:testExecutable = $env:GODOT
        # For running with godot binary, we need to add these flags...
        $script:testArgs += " --disable-crash-handler --headless --path project --"
    }
    # Validate executable
    if (-not (Test-Path $script:testExecutable))
    {
        throw "Executable not found at: $script:testExecutable"
    }

    # Initialize DSN
    $script:testDsn = $env:SENTRY_TEST_DSN
    if ([string]::IsNullOrEmpty($script:testDsn))
    {
        # Read DSN from project.godot as fallback
        $projectGodotPath = Join-Path $PSScriptRoot "../project/project.godot"
        if (Test-Path $projectGodotPath)
        {
            Write-Warning "SENTRY_TEST_DSN environment variable is not set. Reading DSN from project.godot..."
            $projectContent = Get-Content $projectGodotPath -Raw
            if ($projectContent -match 'options/dsn="([^"]+)"')
            {
                $script:testDsn = $matches[1]
            } else
            {
                throw "Could not find DSN in project.godot file"
            }
        } else
        {
            throw "Could not find project.godot file at $projectGodotPath"
        }
    }

    # Initialize Auth token
    $authToken = $env:SENTRY_AUTH_TOKEN
    if ([string]::IsNullOrEmpty($authToken))
    {
        throw "SENTRY_AUTH_TOKEN environment variable is not set."
    }

    # Initialize platform
    $script:testPlatform = $env:SENTRY_TEST_PLATFORM
    if ([string]::IsNullOrEmpty($script:testPlatform))
    {
        Write-Warning "SENTRY_TEST_PLATFORM environment variable is not set. Defaulting to 'Local'."
        $script:testPlatform = "Local"
    }

    Connect-SentryApi `
        -ApiToken $authToken `
        -DSN $testDsn

    Connect-Device -Platform $testPlatform
}

AfterAll {
    Write-Debug "AFTER-ALL"
    Disconnect-SentryApi
}

# Shared test cases among test suites ("Context" items further below)
$CommonTestCases = @(
    @{ Name = 'Has correct release version'; TestBlock = {
            param($SentryEvent)
            $SentryEvent.release.version | Should -Be 'test-app@1.0.0'
        }
    }
    @{ Name = 'Has correct platform'; TestBlock = {
            param($SentryEvent, $TestType)
            $SentryEvent.platform | Should -Not -BeNullOrEmpty
            if ($IsLinux -or $IsWindows) {
                $SentryEvent.platform | Should -Be "native"
            } elseif ($IsMacOS) {
                $SentryEvent.platform | Should -Be "cocoa"
            }
        }
    }
    @{ Name = 'Has correct integration test tags'; TestBlock = {
            param($SentryEvent, $TestType)
            ($SentryEvent.tags | Where-Object { $_.key -eq 'test.suite' }).value | Should -Be 'integration'
            ($SentryEvent.tags | Where-Object { $_.key -eq 'test.type' }).value | Should -Be $TestType
        }
    }
    @{ Name = 'Has correct environment tag'; TestBlock = {
            param($SentryEvent, $TestType)
            ($SentryEvent.tags | Where-Object { $_.key -eq 'environment' }).value | Should -Be 'integration-test'
        }
    }
    @{ Name = 'Has correct OS tag'; TestBlock = {
            param($SentryEvent, $TestType)
            if ($IsLinux) {
                $expectedOS = "Linux"
            } elseif ($IsMacOS) {
                $expectedOS = "macOS"
            } else {
                $expectedOS = "Windows"
            }
            ($SentryEvent.tags | Where-Object { $_.key -eq 'os' }).value | Should -Match $expectedOS
        }
    }
)

Describe "Desktop Integration Tests" {

    Context "Crash Capture" {
        BeforeAll {
            Write-Host "Testing crash-capture..."

            # ACT: Run crash-capture action in test application
            Write-GitHub "::group::Log of crash-capture"
            $runResult = Invoke-DeviceApp -ExecutablePath $testExecutable -Arguments ($testArgs + " crash-capture")
            Write-GitHub "::endgroup::"

            # Save result in a file
            $runResult | ConvertTo-Json -Depth 5 | Out-File -FilePath (Get-OutputFilePath 'crash-capture-result.json')

            $eventId = Get-EventIds -appOutput $runResult.Output -expectedCount 1
            if ($eventId)
            {
                # Retrieve the Sentry event associated with the crash ID,
                # which will be tested in the following "It" blocks.
                $script:runEvent = Get-SentryTestEvent -TagName 'test.crash_id' -TagValue "$eventId" -TimeoutSeconds 120
            }
        }

        It "Exits with non-zero code" {
            $runResult.ExitCode | Should -Not -Be 0
        }

        It "Outputs event ID" {
            $crashId = Get-EventIds -appOutput $runResult.Output -expectedCount 1
            $crashId | Should -Not -BeNullOrEmpty
        }

        It "Completes pre-crash setup" {
            ($runResult.Output | Where-Object { $_ -match 'TEST_RESULT:.*crash-capture.*Pre-crash setup complete' }) | Should -Not -BeNullOrEmpty
        }

        It "Triggers a crash" {
            ($runResult.Output | Where-Object { $_ -match 'Triggering controlled crash' }) | Should -Not -BeNullOrEmpty
        }

        It "Captures event in sentry.io" {
            $runEvent | Should -Not -BeNullOrEmpty
        }

        It "Has required attributes" {
            $runEvent.title | Should -Not -BeNullOrEmpty
            # NOTE: Commented out, because debug symbols are needed
            # $runEvent.title | Should -Not -Be "<unknown>"
            $runEvent.type | Should -Be "error"
        }

        It "Has proper platform" {
            $runEvent.platform | Should -Not -BeNullOrEmpty
            if ($IsLinux -or $IsWindows) {
                $runEvent.platform | Should -Be "native"
            } elseif ($IsMacOS) {
                $runEvent.platform | Should -Be "cocoa"
            }
        }

        It "Has optional attributes" {
            $runEvent.level | Should -Be "fatal"  # This is tag
            $runEvent.logger | Should -Be "lala"
            $runEvent.release | Should -Be "my-game@1.0.0"
            $runEvent.dist | Should -Be "test-dist"
            $runEvent.environment | Should -Be "testing"
        }

        It "Has expected crash tags" {
            ($runEvent.tags | Where-Object { $_.key -eq 'level' }).value | Should -Be 'fatal'
            ($runEvent.tags | Where-Object { $_.key -eq 'mechanism' }).value | Should -Not -BeNullOrEmpty
        }

        It "Has integration test tag" {
            ($runEvent.tags | Where-Object { $_.key -eq 'test.suite' }).value | Should -Be 'integration'
        }

        It "Contains exception information" {
            $runEvent.exception | Should -Not -BeNullOrEmpty
            $runEvent.exception.values | Should -Not -BeNullOrEmpty
        }

        It "Contains exception value with stack trace" {
            $exception = $runEvent.exception.values[0]
            $exception | Should -Not -BeNullOrEmpty
            $exception.type | Should -Match 'SIGSEGV'
            $exception.stacktrace | Should -Not -BeNullOrEmpty
            $exception.threadId | Should -Not -BeNullOrEmpty
        }

        It "Contains stacktrace frames" {
            $frames = $runEvent.exception.values[0].stacktrace.frames
            $frames | Should -Not -BeNullOrEmpty
            $frames.Count | Should -BeGreaterThan 5
        }

        It "Contains expected frames" {
            # TODO: validate presence of specific frames
        }

        It "Contains threads information" {
            $runEvent.threads | Should -Not -BeNullOrEmpty
            $runEvent.threads.values | Should -Not -BeNullOrEmpty
        }

        It "Contains crashed thread with threadId" {
            $threadId = $runEvent.threads.values | Where-Object { $_.crashed -eq $true } | Select-Object -ExpandProperty id
            $threadId | Should -Not -BeNullOrEmpty
        }

        It "Contains user information" {
            $runEvent.user | Should -Not -BeNullOrEmpty
            $runEvent.user.username | Should -Be 'TestUser'
            $runEvent.user.email | Should -Be 'user-mail@test.abc'
            $runEvent.user.id | Should -Be '12345'
        }

        It "Contains breadcrumbs" {
            $runEvent.breadcrumbs | Should -Not -BeNullOrEmpty
            $runEvent.breadcrumbs.values | Should -Not -BeNullOrEmpty
        }

        It '<Name>' -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType 'crash-capture'
        }
    }

    Context "Message Capture" {
        BeforeAll {
            $script:TEST_MESSAGE = "Test message"

            # ACT: Run message-capture action in test application
            Write-GitHub "::group::Log of message-capture"
            $arguments = $testArgs + " message-capture " + "`"$TEST_MESSAGE`""
            $runResult = Invoke-DeviceApp -ExecutablePath $testExecutable -Arguments $arguments
            Write-GitHub "::endgroup::"

            $runResult | ConvertTo-Json -Depth 5 | Out-File -FilePath (Get-OutputFilePath 'message-capture-result.json')

            $eventId = Get-EventIds -AppOutput $runResult.Output -ExpectedCount 1
            Write-Host $eventId
            if ($eventId)
            {
                $script:runEvent = Get-SentryTestEvent -EventId "$eventId"
            }
        }

        It "Exits with code zero" {
            $runResult.ExitCode | Should -Be 0
            $runEvent | ConvertTo-Json -Depth 20 | Out-File -FilePath "message.json"
        }

        It "Has title" {
            $runEvent.title | Should -Not -BeNullOrEmpty
            $runEvent.title | Should -Be $TEST_MESSAGE
        }

        It "Has formatted message" {
            $runEvent.message | Should -Not -BeNullOrEmpty
            $runEvent.message.formatted | Should -Not -BeNullOrEmpty
            $runEvent.message.formatted | Should -Be $TEST_MESSAGE
        }

        It '<Name>' -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType 'message-capture'
        }
    }

    Context "Runtime Error Capture" {
        # TODO: Capture and validate Godot runtime error
        # TODO: Validate script context
    }
}
