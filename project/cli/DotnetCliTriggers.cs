using Godot;
using System;
using System.Collections.Generic;
using Sentry;
using Sentry.Godot;

/// <summary>
/// Helpers used by GDScript CLI commands to drive .NET integration tests.
/// </summary>
public partial class DotnetCliTriggers : RefCounted
{
    public void InitSentryFromDotnet()
    {
        SentrySdk.Init(options =>
        {
            options.Debug = false;
            options.Release = "test-app@1.0.0";
            options.Environment = "integration-test";
            options.Distribution = "test-dist";
            options.AttachLog = true;
            options.AttachSceneTree = true;
            options.AttachScreenshot = false; // CI runs headless
        });
    }

    public void CloseSentryFromDotnet()
    {
        SentrySdk.Close();
    }

    private int _nativeEventsDroppedInBeforeSend;

    /// <summary>
    /// Initializes Sentry with the native before-send hook, for the integration test that verifies the
    /// hook fires and can inspect, mutate, and drop native (GDScript and engine) events.
    /// </summary>
    public void InitSentryWithNativeBeforeSend()
    {
        SentrySdk.Init(options =>
        {
            options.Debug = false;
            options.Release = "test-app@1.0.0";
            options.Environment = "integration-test";
            options.Distribution = "test-dist";
            options.AttachLog = true;
            options.AttachSceneTree = true;
            options.AttachScreenshot = false; // CI runs headless
            options.Native.SetBeforeSend(OnBeforeSendNative);
        });
    }

    /// <summary>
    /// Native before-send callback for the integration test. It discards events carrying the DROP_ME
    /// sentinel, prints every getter's value as a "BEFORE_SEND_*" line the test parses from stdout,
    /// then overrides every setter so the test can verify each change on the event fetched from Sentry.
    /// </summary>
    private SentryNativeEvent OnBeforeSendNative(SentryNativeEvent ev)
    {
        if (ev.Message is not null && ev.Message.Contains("DROP_ME"))
        {
            _nativeEventsDroppedInBeforeSend++;
            return null;
        }

        // The test drives three native events through this callback: the drop sentinel discarded above, a
        // runtime error (push_error) that carries an exception, and a message (capture_message) that does not.
        if (ev.ExceptionCount > 0) // error event
        {
            GD.Print($"BEFORE_SEND_ERR_EXCEPTION_COUNT: {ev.ExceptionCount}");
            GD.Print($"BEFORE_SEND_ERR_EXCEPTION_VALUE: {ev.GetExceptionValue(0)}");

            ev.SetExceptionValue(0, $"[mutated by .NET] {ev.GetExceptionValue(0)}");
            return ev;
        }
        else // message event
        {
            // Getters: print each read so the test verifies the callback saw the right data.
            GD.Print($"BEFORE_SEND_READ_ID: {ev.Id}");
            GD.Print($"BEFORE_SEND_READ_PLATFORM: {ev.Platform}");
            GD.Print($"BEFORE_SEND_READ_MESSAGE: {ev.Message}");
            GD.Print($"BEFORE_SEND_READ_LEVEL: {ev.Level.ToString().ToLowerInvariant()}");
            GD.Print($"BEFORE_SEND_READ_LOGGER: {ev.Logger}");
            GD.Print($"BEFORE_SEND_READ_RELEASE: {ev.Release}");
            GD.Print($"BEFORE_SEND_READ_DIST: {ev.Distribution}");
            GD.Print($"BEFORE_SEND_READ_ENVIRONMENT: {ev.Environment}");
            GD.Print($"BEFORE_SEND_READ_TAG: {ev.GetTag("test.suite")}");
            GD.Print($"BEFORE_SEND_READ_EXCEPTION_COUNT: {ev.ExceptionCount}");
            // Proves the drop branch ran for the earlier sentinel event.
            GD.Print($"BEFORE_SEND_DROPPED_COUNT: {_nativeEventsDroppedInBeforeSend}");

            // Setters: override each, then verify on the event fetched from Sentry.
            ev.Message = "Before-send override: 世界 👋";
            ev.Level = SentryLevel.Warning;
            ev.Logger = "before-send-logger";
            ev.Release = "before-send-release@9.9.9";
            ev.Distribution = "before-send-dist";
            ev.Environment = "before-send-env";
            ev.SetTag("before_send.added", "added-by-native-before-send");
            ev.UnsetTag("before_send.should_be_removed");

            return ev;
        }
    }

    public void AddIntegrationTestContext(string testType)
    {
        SentrySdk.AddBreadcrumb("Integration test started");

        SentrySdk.ConfigureScope(scope =>
        {
            scope.User = new global::Sentry.SentryUser
            {
                Id = "12345",
                Username = "TestUser",
                Email = "user-mail@test.abc",
            };
        });

        SentrySdk.SetTag("test.suite", "integration");
        SentrySdk.SetTag("test.type", testType);

        SentrySdk.AddBreadcrumb("Context configuration finished");
    }

    /// <summary>
    /// Sets .NET-only scope items used by the cross-layer capture test to verify
    /// that these items propagate to native events (tags, breadcrumbs, etc.).
    /// </summary>
    public void AddCrossLayerScopeSyncProbes()
    {
        SentrySdk.SetTag("dotnet.scope.synced", "from-dotnet");
        SentrySdk.SetTag("dotnet.scope.removed", "should-not-appear");
        SentrySdk.UnsetTag("dotnet.scope.removed");
        SentrySdk.AddBreadcrumb("Synced from .NET");

        // Breadcrumb with data exercises the ManagedStringMap marshalling path.
        var data = new Dictionary<string, string>
        {
            ["http.url"] = "https://example.test/api/v1/probe",
            ["http.status"] = "200",
            ["unicode"] = "Hello 世界! 👋",
        };
        SentrySdk.AddBreadcrumb(
            message: "Synced data breadcrumb from .NET",
            category: "dotnet.probe",
            type: "http",
            data: data);

        SentrySdk.ConfigureScope(scope =>
        {
            scope.User = new SentryUser
            {
                Id = "99999",
                Username = "DotnetSyncedUser",
                Email = "dotnet-synced@test.abc",
                IpAddress = "1.2.3.4",
            };
        });

        // Local scope mutations must NOT leak to the native current scope.
        // Verified by assertions in the cross-layer test.
        SentrySdk.CaptureMessage("Local scope probe", scope =>
        {
            scope.SetTag("dotnet.local_scope.tag", "should_not_leak");
            scope.AddBreadcrumb("Local scope leak breadcrumb");
            scope.UnsetTag("dotnet.scope.synced");
            scope.User = new SentryUser
            {
                Id = "should_not_leak",
                Username = "should_not_leak",
                Email = "should_not_leak@test.abc",
            };
        });
    }

    public string GetLastEventId()
    {
        return SentrySdk.LastEventId.ToString();
    }

    public string CaptureMessage(string message)
    {
        GD.Print("Capturing message in .NET layer: ", message);
        return SentrySdk.CaptureMessage(message).ToString();
    }

    public bool IsSdkEnabled()
    {
        return SentrySdk.IsEnabled;
    }

    public void TriggerException()
    {
        GD.Print("Triggering exception...");
        throw new Exception("Exception (should be captured)");
    }

    public void TriggerBareRethrow()
    {
        GD.Print("Triggering bare rethrow...");
        try
        {
            throw new Exception("Bare rethrow (should be captured)");
        }
        catch
        {
            throw;
        }
    }

    public void TriggerWrappedRethrow()
    {
        GD.Print("Triggering wrapped rethrow...");
        try
        {
            throw new Exception("Inner exception");
        }
        catch (Exception ex)
        {
            throw new InvalidOperationException("Wrapped exception", ex);
        }
    }
}
