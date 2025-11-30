# Defines a collection of reusable test cases that validate common Sentry event properties
# and behaviors across different test suites for consistent integration testing.
#
# Available parameters:
# - $SentryEvent: The Sentry event object retrieved from the REST API containing error/message details
# - $TestType: String indicating the type of test being run (e.g., "crash-capture", "message-capture")
# - $RunResult: Object containing the results of running the test application, including Output and ExitCode

$CommonTestCases = @(
    @{ Name = "Outputs event ID"; TestBlock = {
            param($RunResult)
            $eventId = Get-EventIds -appOutput $RunResult.Output -expectedCount 1
            $eventId | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Captures event in sentry.io"; TestBlock = {
            param($SentryEvent)
            $SentryEvent | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Has title"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.title | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Has correct release version"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.release.version | Should -Be "test-app@1.0.0"
        }
    }
    @{ Name = "Has correct platform"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.platform | Should -Not -BeNullOrEmpty
            if ($IsLinux -or $IsWindows) {
                $SentryEvent.platform | Should -Be "native"
            } elseif ($IsMacOS) {
                $SentryEvent.platform | Should -Be "cocoa"
            }
        }
    }
    @{ Name = "Has correct dist attribute"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.dist | Should -Be "test-dist"
        }
    }
    @{ Name = "Has tags"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.tags | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Has correct integration test tags"; TestBlock = {
            param($SentryEvent, $TestType)
            ($SentryEvent.tags | Where-Object { $_.key -eq "test.suite" }).value | Should -Be "integration"
            ($SentryEvent.tags | Where-Object { $_.key -eq "test.type" }).value | Should -Be $TestType
        }
    }
    @{ Name = "Has correct environment tag"; TestBlock = {
            param($SentryEvent)
            ($SentryEvent.tags | Where-Object { $_.key -eq "environment" }).value | Should -Be "integration-test"
        }
    }
    @{ Name = "Has correct OS tag"; TestBlock = {
            param($SentryEvent)
            if ($IsLinux) {
                $expectedOS = "Linux"
            } elseif ($IsMacOS) {
                $expectedOS = "macOS"
            } else {
                $expectedOS = "Windows"
            }
            ($SentryEvent.tags | Where-Object { $_.key -eq "os" }).value | Should -Match $expectedOS
        }
    }
    @{ Name = "Contains user information"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.user | Should -Not -BeNullOrEmpty
            $SentryEvent.user.username | Should -Be "TestUser"
            $SentryEvent.user.email | Should -Be "user-mail@test.abc"
            $SentryEvent.user.id | Should -Be "12345"
        }
    }
    @{ Name = "Contains breadcrumbs"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.breadcrumbs | Should -Not -BeNullOrEmpty
            $SentryEvent.breadcrumbs.values | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains expected breadcrumbs"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.breadcrumbs.values | Should -Not -BeNullOrEmpty
            $SentryEvent.breadcrumbs.values | Where-Object { $_.message -eq "Integration test started" } | Should -Not -BeNullOrEmpty
            $SentryEvent.breadcrumbs.values | Where-Object { $_.message -eq "Context configuration finished" } | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains SDK information"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.sdk | Should -Not -BeNullOrEmpty
            $SentryEvent.sdk.name | Should -Not -BeNullOrEmpty
            $SentryEvent.sdk.version | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains app context"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.contexts.app | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.app.app_name | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.app.app_version | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains device context"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.contexts.device | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.device.arch | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.device.free_memory | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.device.usable_memory | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains OS context"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.contexts.os | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.os.os | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.os.name | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.os.version | Should -Not -BeNullOrEmpty
        }
    }
    @{ Name = "Contains Godot contexts"; TestBlock = {
            param($SentryEvent)
            $SentryEvent.contexts.godot_engine | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.godot_engine.version | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.godot_engine.version_commit | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.godot_engine.godot_sdk_version | Should -Not -BeNullOrEmpty
            $SentryEvent.contexts.godot_performance | Should -Not -BeNullOrEmpty
        }
    }
)
