using System;
using Sentry.Integrations;
using Sentry.Godot.Interop;

namespace Sentry.Godot.Internal;

internal sealed class GodotSdkIntegration : ISdkIntegration
{
    public void Register(IHub hub, SentryOptions options)
    {
        hub.ConfigureScope(scope =>
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
        });
    }
}
