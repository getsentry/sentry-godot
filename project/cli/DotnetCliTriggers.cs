using Godot;
using System;

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

    public string GetLastEventId()
    {
        return Sentry.Godot.SentrySdk.LastEventId.ToString();
    }

    public string CaptureMessage(string message)
    {
        GD.Print("Capturing message in .NET layer: ", message);
        return Sentry.Godot.SentrySdk.CaptureMessage(message).ToString();
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
