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
        Sentry.Godot.SentrySdk.Init(options =>
        {
            options.Debug = false;
            options.Release = "test-app@1.0.0";
            options.Environment = "integration-test";
            options.Distribution = "test-dist";
        });
    }

    public void AddIntegrationTestContext(string testType)
    {
        Sentry.Godot.SentrySdk.AddBreadcrumb("Integration test started");

        Sentry.Godot.SentrySdk.ConfigureScope(scope =>
        {
            scope.User = new global::Sentry.SentryUser
            {
                Id = "12345",
                Username = "TestUser",
                Email = "user-mail@test.abc",
            };
        });

        Sentry.Godot.SentrySdk.SetTag("test.suite", "integration");
        Sentry.Godot.SentrySdk.SetTag("test.type", testType);

        Sentry.Godot.SentrySdk.AddBreadcrumb("Context configuration finished");
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
        return Sentry.Godot.SentrySdk.LastEventId.ToString();
    }

    public string CaptureMessage(string message)
    {
        GD.Print("Capturing message in .NET layer: ", message);
        return Sentry.Godot.SentrySdk.CaptureMessage(message).ToString();
    }

    public bool IsSdkEnabled()
    {
        return Sentry.Godot.SentrySdk.IsEnabled;
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
