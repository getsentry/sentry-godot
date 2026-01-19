# Defines a collection of reusable test cases that validate common Sentry event properties
# and behaviors across different test suites for consistent integration testing.
#
# Available parameters:
# - $TestSetup: Object containing test setup parameters
# - $TestType: String indicating the type of test being run (e.g., "crash-capture", "message-capture")
# - $SentryEvent: The Sentry event object retrieved from the REST API containing error/message details
# - $RunResult: Object containing the results of running the test application, including Output and ExitCode

$CommonTestCases = @(
    @{ Name = "Outputs event ID"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $eventId = Get-EventIds -appOutput $RunResult.Output -expectedCount 1
            $eventId | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Captures event in sentry.io"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Has title"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.title | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Has correct release version"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.release.version | Should -Be "test-app@1.0.0"
        }
    }
    @{ Name = "Has correct platform"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.platform | Should -Not -BeNullOrEmpty

            $expectedPlatform = @{
                "Windows" = "native"
                "Linux" = "native"
                "macOS" = "cocoa"
            }

            if ($expectedPlatform.ContainsKey($TestSetup.Platform)) {
                $SentryEvent.platform | Should -Be $expectedPlatform[$TestSetup.Platform]
            }
        }
    }
    @{ Name = "Has correct dist attribute"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.dist | Should -Be "test-dist"
        }
    }
    @{ Name = "Has tags"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.tags | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Has correct integration test tags"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            ($SentryEvent.tags | Where-Object { $_.key -eq "test.suite" }).value | Should -Be "integration"
            ($SentryEvent.tags | Where-Object { $_.key -eq "test.type" }).value | Should -Be $TestType
        }
    }
    @{ Name = "Has correct environment tag"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            ($SentryEvent.tags | Where-Object { $_.key -eq "environment" }).value | Should -Be "integration-test"
        }
    }
    @{ Name = "Has correct OS tag"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            if ($TestSetup.Platform -ieq "Linux") {
                $expectedOS = "Linux"
            } elseif ($TestSetup.Platform -ieq "macOS") {
                $expectedOS = "macOS"
            } elseif ($TestSetup.Platform -ieq "Windows") {
                $expectedOS = "Windows"
            } elseif ($TestSetup.IsAndroid) {
                $expectedOS = "Android"
            } elseif ($TestSetup.Platform -match "iOS") {
                $expectedOS = "iOS"
            }
            ($SentryEvent.tags | Where-Object { $_.key -eq "os" }).value | Should -Match $expectedOS
        }
    }
    @{ Name = "Contains user information"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.user | Should -Not -BeNullOrEmpty
            $SentryEvent.user.username | Should -Be "TestUser"
            $SentryEvent.user.email | Should -Be "user-mail@test.abc"
            $SentryEvent.user.id | Should -Be "12345"
        }
    }
    @{ Name = "Contains breadcrumbs"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.breadcrumbs | Should -Not -BeNullOrEmpty
            $SentryEvent.breadcrumbs.values | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains expected breadcrumbs"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.breadcrumbs.values | Should -Not -BeNullOrEmpty
            $SentryEvent.breadcrumbs.values | Where-Object { $_.message -eq "Integration test started" } | Should -Not -BeNullOrEmpty
            $SentryEvent.breadcrumbs.values | Where-Object { $_.message -eq "Context configuration finished" } | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains SDK information"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.sdk | Should -Not -BeNullOrEmpty
            $SentryEvent.sdk.name | Should -Not -BeNullOrEmpty
            $SentryEvent.sdk.version | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains app context"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)

            if ($TestSetup.IsAndroid -and $TestType -eq "crash-capture") {
                # Skip app context check for Android crashes
                # Q: Bug?
                return
            }

            $SentryEvent.contexts.app | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.app.app_name | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.app.app_version | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains device context"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.contexts.device | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains OS context"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)
            $SentryEvent.contexts.os | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.os.os | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.os.name | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.os.version | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains Godot contexts"; TestBlock = {
            param($TestSetup, $TestType, $SentryEvent, $RunResult)

            if ($TestSetup.IsAndroid -and $TestType -eq "crash-capture") {
                # Skip Godot context tests for Android crashes
                # NOTE: Contexts don't seem to be synchronized to NDK. Bug?
                return
            }

            $SentryEvent.contexts.godot_engine | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.godot_engine.version | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.godot_engine.version_commit | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.godot_engine.godot_sdk_version | Should -Not -BeNullOrEmpty

            if ($TestSetup.IsCocoa -and $TestType -eq "crash-capture") {
                # Performance context is excluded from crash events on Apple platforms because crash
                # processing occurs in a subsequent session where such data is unavailable.
                $SentryEvent.contexts.godot_performance | Should -BeNullOrEmpty
            } else {
            	$SentryEvent.contexts.godot_performance | Should -Not -BeNullOrEmpty
            }
        }
    }
)
