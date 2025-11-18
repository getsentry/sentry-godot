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

# Import shared test cases
. $PSScriptRoot/CommonTestCases.ps1

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


Describe "Platform Integration Tests" {
    # TODO: structured logs tests
    # TODO: user feedback tests
    # TODO: attachment tests: screenshot, VH, log file, and custom attachments

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

        # Include shared test cases from CommonTestCases.ps1
        It '<Name>' -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType 'crash-capture' -RunResult $runResult
        }

        It "Exits with non-zero code" {
            $runResult.ExitCode | Should -Not -Be 0
        }

        It "Completes pre-crash setup" {
            ($runResult.Output | Where-Object { $_ -match 'TEST_RESULT:.*crash-capture.*Pre-crash setup complete' }) | Should -Not -BeNullOrEmpty
        }

        It "Triggers a crash" {
            ($runResult.Output | Where-Object { $_ -match 'Triggering controlled crash' }) | Should -Not -BeNullOrEmpty
        }

        It "Has correct type" {
            $runEvent.type | Should -Be "error"
        }

        It "Has expected level tag" {
            ($runEvent.tags | Where-Object { $_.key -eq 'level' }).value | Should -Be 'fatal'
        }

        It "Contains mechanism tag" {
            ($runEvent.tags | Where-Object { $_.key -eq 'mechanism' }).value | Should -Not -BeNullOrEmpty
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
            $threadId = $runEvent.threads.values | Where-Object { $_.crashed -eq $true } | Select-Object -ExpandProperty id
            $threadId | Should -Not -BeNullOrEmpty
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

        # Include shared test cases from CommonTestCases.ps1
        It '<Name>' -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType 'message-capture' -RunResult $runResult
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

        It "Has expected level tag" {
            ($runEvent.tags | Where-Object { $_.key -eq 'level' }).value | Should -Be 'info'
        }
    }

    Context "Runtime Error Capture" {
        # TODO: Test local variables
        # TODO: Test exact script source context (should we?)
        BeforeAll {
            # ACT: Run runtime-error-capture action in test application
            Write-GitHub "::group::Log of runtime-error-capture"
            $arguments = $testArgs + " runtime-error-capture"
            $runResult = Invoke-DeviceApp -ExecutablePath $testExecutable -Arguments $arguments
            Write-GitHub "::endgroup::"

            $runResult | ConvertTo-Json -Depth 5 | Out-File -FilePath (Get-OutputFilePath 'runtime-error-capture-result.json')

            $eventId = Get-EventIds -AppOutput $runResult.Output -ExpectedCount 1
            Write-Host $eventId
            if ($eventId)
            {
                $script:runEvent = Get-SentryTestEvent -EventId "$eventId"
            }
        }

        # Include shared test cases from CommonTestCases.ps1
        It '<Name>' -ForEach $CommonTestCases {
            & $testBlock -SentryEvent $runEvent -TestType 'runtime-error-capture' -RunResult $runResult
        }

        It 'Exits with code zero' {
            $runResult.ExitCode | Should -Be 0
        }

        It 'Triggers runtime error' {
            ($runResult.Output | Where-Object { $_ -match 'Triggering runtime error' }) | Should -Not -BeNullOrEmpty
        }

        It 'Outputs stack trace frames in correct format' {
            $frameLines = $runResult.Output | Where-Object { $_ -match '^FRAME: ' }
            $frameLines | Should -Not -BeNullOrEmpty
            $frameLines.Count | Should -BeGreaterThan 0

            # Validate frame format: "FRAME: {file} | {function} | {line}"
            foreach ($frame in $frameLines) {
                $frame | Should -Match '^FRAME: res://.*\.gd \| \w+ \| \d+$'
            }
        }

        It 'Has correct type' {
            $runEvent.type | Should -Be "error"
        }

        It 'Has expected level tag' {
            ($runEvent.tags | Where-Object { $_.key -eq 'level' }).value | Should -Be 'error'
        }

        It 'Has correct logger tag' {
            ($runEvent.tags | Where-Object { $_.key -eq 'logger' }).value | Should -Be 'SentryGodotLogger'
        }

        It 'Contains correct exception data' {
            $runEvent.exception | Should -Not -BeNullOrEmpty
            $runEvent.exception.values | Should -HaveCount 1
            $exception = $runEvent.exception.values[0]
            $exception | Should -Not -BeNullOrEmpty
            $exception.type | Should -Not -BeNullOrEmpty
            $exception.value | Should -Be 'Runtime error'
        }

        It 'Contains threads data' {
            # Validate that Sentry event contains corresponding stack information
            $runEvent.threads | Should -Not -BeNullOrEmpty
            $runEvent.threads.values | Should -Not -BeNullOrEmpty
        }

        It 'Has threads with stacktrace frames' {
            $runEvent.threads | Should -Not -BeNullOrEmpty
            $runEvent.threads.values | Should -Not -BeNullOrEmpty

            # Find at least one thread with stacktrace frames
            $threadsWithFrames = $runEvent.threads.values | Where-Object {
                $_.stacktrace -and $_.stacktrace.frames
            }
            $threadsWithFrames | Should -Not -BeNullOrEmpty
            $threadsWithFrames.Count | Should -BeGreaterThan 0
        }

        It 'Has threads with GDScript frames with proper attributes' {
            # Find threads with stacktrace frames
            $threadsWithFrames = $runEvent.threads.values | Where-Object {
                $_.stacktrace -and $_.stacktrace.frames
            }

            $gdscriptFramesFound = $false
            foreach ($thread in $threadsWithFrames) {
                $gdscriptFrames = $thread.stacktrace.frames | Where-Object { $_.platform -eq 'gdscript' }
                if ($gdscriptFrames -and $gdscriptFrames.Count -gt 0) {
                    $gdscriptFramesFound = $true

                    # Validate each GDScript frame has required attributes
                    foreach ($frame in $gdscriptFrames) {
                        $frame.filename | Should -Not -BeNullOrEmpty
                        $frame.function | Should -Not -BeNullOrEmpty
                        $frame.lineNo | Should -BeGreaterThan 0
                        $frame.platform | Should -Be 'gdscript'
                        $frame.inApp | Should -BeTrue
                        $frame.context.Count | Should -Be 11  # 1 current line + 5 before + 5 after == 11
                    }
                    break
                }
            }
            $gdscriptFramesFound | Should -Be $true -Because "At least one GDScript frame should be present in stacktrace"
        }

        It 'Has GDScript frames matching expected output frames and order' {
            # Parse FRAME lines from output
            $frameLines = $runResult.Output | Where-Object { $_ -match '^FRAME: ' }
            $frameLines | Should -Not -BeNullOrEmpty

            # Parse expected frame information from output (in order)
            $expectedFrames = @()
            foreach ($line in $frameLines) {
                if ($line -match '^FRAME: ([^|]+) \| ([^|]+) \| (\d+)$') {
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

            $gdscriptFrames = $currentThread.stacktrace.frames | Where-Object { $_.platform -eq 'gdscript' }
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
