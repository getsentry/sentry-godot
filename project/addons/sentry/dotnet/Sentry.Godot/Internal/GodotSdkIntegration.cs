using System;
using Sentry.Integrations;
using Sentry.Godot.Interop;
using Scope = Sentry.Scope;

namespace Sentry.Godot.Internal;

internal sealed class GodotSdkIntegration : ISdkIntegration
{
    public void Register(IHub hub, SentryOptions options)
    {
        AdoptNativeTrace(hub);
        hub.ConfigureScope(ApplyScopeChanges);
    }

    private static void AdoptNativeTrace(IHub hub)
    {
        var (traceId, parentSpanId) = NativeBridge.GetTraceContext();
        if (!string.IsNullOrEmpty(traceId))
        {
            hub.ContinueTrace(
                new SentryTraceHeader(
                    SentryId.Parse(traceId),
                    string.IsNullOrEmpty(parentSpanId) ? SpanId.Empty : SpanId.Parse(parentSpanId),
                    isSampled: null),
                baggageHeader: null);
        }
    }

    private static void ApplyScopeChanges(Scope scope)
    {
        scope.Sdk.Name = "sentry.dotnet.godot";
        scope.Sdk.Version = NativeBridge.GetSdkVersion();
        if (scope.Contexts.App.Name is null)
        {
            string appName = NativeBridge.GetAppName();
            if (appName.Length > 0)
            {
                scope.Contexts.App.Name = appName;
            }
        }
        if (scope.Contexts.App.Version is null)
        {
            string appVersion = NativeBridge.GetAppVersion();
            if (appVersion.Length > 0)
            {
                scope.Contexts.App.Version = appVersion;
            }
        }

        AddDefaultAttachments(scope);
    }

    private static void AddDefaultAttachments(Scope scope)
    {
        // Don't propagate these scope changes to the native layer.
        using var _ = new GodotScopeObserver.SyncGuard();
        foreach (SentryAttachment att in Sentry.Godot.SentrySdk.CurrentOptions!.DefaultAttachments)
        {
            scope.AddAttachment(att);
        }
    }
}
