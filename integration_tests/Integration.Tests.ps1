#!/usr/bin/env pwsh
#
# Integration tests for Sentry Godot SDK
#
# Environment variables:
#   SENTRY_TEST_EXECUTABLE: command line to run the project
#   SENTRY_TEST_ARGS: arguments
#   SENTRY_TEST_DSN: test DSN
#   SENTRY_AUTH_TOKEN: authentication token for Sentry API
#   SENTRY_TEST_PLATFORM: test platform (aka device provider) such as "Local" or "Android"

Set-StrictMode -Version latest
$ErrorActionPreference = "Stop"
$global:DebugPreference = "Continue"

# Import app-runner modules
. $PSScriptRoot/../modules/app-runner/import-modules.ps1

# Import shared test cases
. $PSScriptRoot/CommonTestCases.ps1

# Import utility functions
. $PSScriptRoot/Utils.ps1


BeforeAll {
    # Run integration test action on device
    function Invoke-TestAction {
        param (
            [Parameter(Mandatory=$true)]
            [string]$Action,
            [string]$AdditionalArgs = ""
        )

        # ACT: Run test action in application on device
        Write-Debug "Running $Action..."
        $arguments = $script:TestSetup.Args + " $Action $AdditionalArgs"
        $execPath = $script:TestSetup.Executable

        # Convert arguments to Android extras if necessary
        if ($script:TestSetup.IsAndroid) {
            $arguments = ConvertTo-AndroidExtras -Arguments $arguments
            $execPath = $script:TestSetup.AndroidComponent
            Write-Host "Using arguments $arguments"
        }

        $runResult = Invoke-DeviceApp -ExecutablePath $execPath -Arguments $arguments

        # Save result to JSON file
        $runResult | ConvertTo-Json -Depth 5 | Out-File -FilePath (Get-OutputFilePath "${Action}-result.json")

        # Launch app again to ensure crash report is sent
        # NOTE: On Cocoa & Android, crashes are sent during the next app launch.
        if (
            ($Action -eq "crash-capture" -or $runResult.ExitCode -ne 0) -and
                $script:TestSetup.Platform -in @("macOS", "Local", "Adb", "AndroidSauceLabs")
        ) {
            Write-Debug "Running crash-send to ensure crash report is sent..."
            Write-GitHub "::group::Log of crash-send"
            $arguments = ($script:TestSetup.Args + " crash-send")
            if ($script:TestSetup.IsAndroid) {
                $arguments = ConvertTo-AndroidExtras -Arguments $arguments
            }
            Invoke-DeviceApp -ExecutablePath $execPath -Arguments $arguments
            Write-GitHub "::endgroup::"
        }

        return $runResult
    }

    # Create directory for the test results
    New-Item -ItemType Directory -Path "$PSScriptRoot/results/" -ErrorAction Continue 2>&1 | Out-Null
    Set-OutputDir -Path "$PSScriptRoot/results/"

    # Initialize test parameters object
    $script:TestSetup = [PSCustomObject]@{
        Executable = $env:SENTRY_TEST_EXECUTABLE
        Args = $env:SENTRY_TEST_ARGS
        Dsn = $env:SENTRY_TEST_DSN
        AuthToken = $env:SENTRY_AUTH_TOKEN
        Platform = $env:SENTRY_TEST_PLATFORM
        AndroidComponent = "io.sentry.godot.project/com.godot.game.GodotApp"
        IsAndroid = ($env:SENTRY_TEST_PLATFORM -in @("Adb", "AndroidSauceLabs"))
    }

    # Check executable and arguments
    if ([string]::IsNullOrEmpty($script:TestSetup.Executable)) {
        Write-Warning "SENTRY_TEST_EXECUTABLE environment variable is not set. Defaulting to env:GODOT."
        $script:TestSetup.Executable = $env:GODOT
        # For running with Godot binary, we need to add these flags...
        $script:TestSetup.Args += " --disable-crash-handler --headless --path project --"
    }
    # Validate executable
    if (-not (Test-Path $script:TestSetup.Executable)) {
        throw "Executable not found at: $($script:TestSetup.Executable)"
    }

    # Check DSN
    if ([string]::IsNullOrEmpty($script:TestSetup.Dsn)) {
        # Read DSN from project.godot as fallback
        $projectGodotPath = Join-Path $PSScriptRoot "../project/project.godot"
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

    # Check auth token
    if ([string]::IsNullOrEmpty($script:TestSetup.AuthToken)) {
        throw "SENTRY_AUTH_TOKEN environment variable is not set."
    }

    # Check platform
    if ([string]::IsNullOrEmpty($script:TestSetup.Platform)) {
        Write-Warning "SENTRY_TEST_PLATFORM environment variable is not set. Defaulting to 'Local'."
        $script:TestSetup.Platform = "Local"
    }

    Connect-SentryApi `
        -ApiToken $script:TestSetup.AuthToken `
        -DSN $script:TestSetup.Dsn

    Connect-Device -Platform $script:TestSetup.Platform
    Install-DeviceApp -Path (Resolve-Path $script:TestSetup.Executable).Path
}


AfterAll {
    Disconnect-SentryApi
    Disconnect-Device
}


Describe "Platform Integration Tests" {
    # TODO: structured logs tests
    # TODO: user feedback tests
    # TODO: attachment tests: screenshot, VH, log file, and custom attachments

    Context "Crash Capture" {
        BeforeAll {
            Write-Host "Testing crash-capture..."

            $runResult = Invoke-TestAction -Action "crash-capture"

            $eventId = Get-EventIds -appOutput $runResult.Output -expectedCount 1
            if ($eventId) {
                # Retrieve the Sentry event associated with the crash ID,
                # which will be tested in the following "It" blocks.
                Write-GitHub "::group::Getting event content"
                $script:runEvent = Get-SentryTestEvent -TagName "test.crash_id" -TagValue "$eventId" -TimeoutSeconds 120
                Write-GitHub "::endgroup::"
            }
        }

        # Include shared test cases from CommonTestCases.ps1
        It "<Name>" -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType "crash-capture" -RunResult $runResult -TestSetup $script:TestSetup
        }

        It "Exits with non-zero code" {
            if ($TestSetup.IsAndroid) {
                # We don't detect exit code on Android - it's always zero.
                return
            }

            $runResult.ExitCode | Should -Not -Be 0
        }

        It "Completes pre-crash setup" {
            ($runResult.Output | Where-Object { $_ -match "TEST_RESULT:.*crash-capture.*Pre-crash setup complete" }) | Should -Not -BeNullOrEmpty
        }

        It "Triggers a crash" {
            ($runResult.Output | Where-Object { $_ -match "Triggering controlled crash" }) | Should -Not -BeNullOrEmpty
        }

        It "Has correct type" {
            $runEvent.type | Should -Be "error"
        }

        It "Has expected level tag" {
            ($runEvent.tags | Where-Object { $_.key -eq "level" }).value | Should -Be "fatal"
        }

        It "Contains mechanism tag" {
            ($runEvent.tags | Where-Object { $_.key -eq "mechanism" }).value | Should -Not -BeNullOrEmpty
        }

        It "Contains exception information" {
            $runEvent.exception | Should -Not -BeNullOrEmpty
            $runEvent.exception.values | Should -Not -BeNullOrEmpty
        }

        It "Contains exception value with stack trace" {
            $exception = $runEvent.exception.values[0]
            $exception | Should -Not -BeNullOrEmpty
            $exception.type | Should -Not -BeNullOrEmpty
            $exception.stacktrace | Should -Not -BeNullOrEmpty
            # NOTE: null on Android
            # $exception.threadId | Should -Not -BeNullOrEmpty
        }

        It "Contains stacktrace frames" {
            $frames = $runEvent.exception.values[0].stacktrace.frames
            $frames | Should -Not -BeNullOrEmpty
            $frames.Count | Should -BeGreaterThan 0
        }

        It "Contains threads information" {
            if ($script:TestSetup.IsAndroid) {
                # threads info missing on Android
                # Q: Bug?
                return
            }

            $runEvent.threads | Should -Not -BeNullOrEmpty
            $runEvent.threads.values | Should -Not -BeNullOrEmpty
            $threadId = $runEvent.threads.values | Where-Object { $_.crashed -eq $true } | Select-Object -ExpandProperty id
            $threadId | Should -Not -BeNullOrEmpty
        }
    }

    Context "Message Capture" {
        BeforeAll {
            $script:TEST_MESSAGE = "TestMessage"

            $runResult = Invoke-TestAction -Action "message-capture" -AdditionalArgs "`"$TEST_MESSAGE`""

            $eventId = Get-EventIds -AppOutput $runResult.Output -ExpectedCount 1
            if ($eventId) {
               	Write-GitHub "::group::Getting event content"
                $script:runEvent = Get-SentryTestEvent -EventId "$eventId"
                Write-GitHub "::endgroup::"
            }
        }

        # Include shared test cases from CommonTestCases.ps1
        It "<Name>" -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType "message-capture" -RunResult $runResult -TestSetup $script:TestSetup
        }

        It "Exits with code zero" {
            $runResult.ExitCode | Should -Be 0
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

        It "Has expected level tag" {
            ($runEvent.tags | Where-Object { $_.key -eq "level" }).value | Should -Be "info"
        }
    }

    Context "Runtime Error Capture" {
        # TODO: Test local variables
        # TODO: Test exact script source context (should we?)
        BeforeAll {
            $runResult = Invoke-TestAction -Action "runtime-error-capture"

            $eventId = Get-EventIds -AppOutput $runResult.Output -ExpectedCount 1
            if ($eventId) {
                Write-GitHub "::group::Getting event content"
                $script:runEvent = Get-SentryTestEvent -EventId "$eventId"
                Write-GitHub "::endgroup::"
            }
        }

        # Include shared test cases from CommonTestCases.ps1
        It "<Name>" -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType "runtime-error-capture" -RunResult $runResult -TestSetup $script:TestSetup
        }

        It "Exits with code zero" {
            $runResult.ExitCode | Should -Be 0
        }

        It "Triggers runtime error" {
            ($runResult.Output | Where-Object { $_ -match "Triggering runtime error" }) | Should -Not -BeNullOrEmpty
        }

        It "Outputs stack trace frames in correct format" {
            $frameLines = $runResult.Output | Where-Object { $_ -match "FRAME: ([^|]+) \| ([^|]+) \| (\d+)" }
            $frameLines | Should -Not -BeNullOrEmpty
            $frameLines.Count | Should -BeGreaterThan 0

            # Validate frame format: "FRAME: {file} | {function} | {line}"
            foreach ($frame in $frameLines) {
                $frame | Should -Match "FRAME: res://.*\.gd \| \w+ \| \d+$"
            }
        }

        It "Has correct type" {
            $runEvent.type | Should -Be "error"
        }

        It "Has expected level tag" {
            ($runEvent.tags | Where-Object { $_.key -eq "level" }).value | Should -Be "error"
        }

        It "Has correct logger tag" {
            ($runEvent.tags | Where-Object { $_.key -eq "logger" }).value | Should -Be "SentryGodotLogger"
        }

        It "Contains correct exception data" {
            $runEvent.exception | Should -Not -BeNullOrEmpty
            $runEvent.exception.values | Should -HaveCount 1
            $exception = $runEvent.exception.values[0]
            $exception | Should -Not -BeNullOrEmpty
            $exception.type | Should -Not -BeNullOrEmpty
            $exception.value | Should -Be "Runtime error"
        }

        It "Has threads with stacktrace frames" {
            $runEvent.threads | Should -Not -BeNullOrEmpty
            $runEvent.threads.values | Should -Not -BeNullOrEmpty

            # Find at least one thread with stacktrace frames
            $threadsWithFrames = $runEvent.threads.values | Where-Object {
                $_.stacktrace -and $_.stacktrace.frames
            }
            $threadsWithFrames | Should -Not -BeNullOrEmpty
            $threadsWithFrames.Count | Should -BeGreaterThan 0
        }

        It "Has threads with GDScript frames with proper attributes" {
            # Find threads with stacktrace frames
            $threadsWithFrames = $runEvent.threads.values | Where-Object {
                $_.stacktrace -and $_.stacktrace.frames
            }

            $gdscriptFramesFound = $false
            foreach ($thread in $threadsWithFrames) {
                $gdscriptFrames = $thread.stacktrace.frames | Where-Object { $_.platform -eq "gdscript" }
                if ($gdscriptFrames -and $gdscriptFrames.Count -gt 0) {
                    $gdscriptFramesFound = $true

                    # Validate each GDScript frame has required attributes
                    foreach ($frame in $gdscriptFrames) {
                        $frame.filename | Should -Not -BeNullOrEmpty
                        $frame.function | Should -Not -BeNullOrEmpty
                        $frame.lineNo | Should -BeGreaterThan 0
                        $frame.platform | Should -Be "gdscript"
                        $frame.inApp | Should -BeTrue
                        $frame.context.Count | Should -Be 11  # 1 current line + 5 before + 5 after
                    }
                    break
                }
            }
            $gdscriptFramesFound | Should -Be $true -Because "At least one GDScript frame should be present in stacktrace"
        }

        It "Has GDScript frames matching expected output frames and order" {
            # Parse FRAME lines from output
            $frameLines = $runResult.Output | Where-Object { $_ -match "FRAME: ([^|]+) \| ([^|]+) \| (\d+)" }
            $frameLines | Should -Not -BeNullOrEmpty

            # Parse expected frame information from output (in order)
            $expectedFrames = @()
            foreach ($line in $frameLines) {
                if ($line -match "FRAME: ([^|]+) \| ([^|]+) \| (\d+)") {
                    $expectedFrames += @{
                        filename = $matches[1].Trim()
                        function = $matches[2].Trim()
                        line = [int]$matches[3].Trim()
                    }
                }
            }
            $expectedFrames.Count | Should -BeGreaterThan 0

            # Find the current thread and get its GDScript frames
            $currentThread = $runEvent.threads.values | Where-Object { $_.current -eq $true }
            $currentThread | Should -Not -BeNullOrEmpty -Because "Should have a current thread"
            $currentThread.stacktrace | Should -Not -BeNullOrEmpty -Because "Current thread should have stacktrace"
            # NOTE: "crashed" here means the thread which caused the error, not actually crashed.
            $currentThread.crashed | Should -BeTrue

            $gdscriptFrames = $currentThread.stacktrace.frames | Where-Object { $_.platform -eq "gdscript" }
            $gdscriptFrames.Count | Should -BeGreaterThan 0 -Because "Current thread should have GDScript frames"

            # Validate each expected frame exists and save their positions
            $framePositions = @()
            foreach ($expectedFrame in $expectedFrames) {
                $position = -1
                for ($i = 0; $i -lt $gdscriptFrames.Count; $i++) {
                    $frame = $gdscriptFrames[$i]
                    if ($frame.filename -eq $expectedFrame.filename -and
                        $frame.function -eq $expectedFrame.function -and
                        $frame.lineNo -eq $expectedFrame.line) {
                        $position = $i
                        break
                    }
                }
                $position | Should -BeGreaterOrEqual 0 -Because "Frame '$($expectedFrame.function)' should exist in stacktrace"
                $framePositions += $position
            }

            # Validate frame order
            for ($i = 1; $i -lt $framePositions.Count; $i++) {
                $framePositions[$i] | Should -BeGreaterThan $framePositions[$i-1] -Because "Frames should appear in correct order"
            }
        }
    }
}
