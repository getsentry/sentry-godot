using System;
using Sentry.Integrations;
using Sentry.Godot.Interop;

namespace Sentry.Godot.Internal;

internal sealed class GodotSdkIntegration : ISdkIntegration {
	public void Register(IHub hub, SentryOptions options) {
		hub.ConfigureScope(scope => {
			scope.Sdk.Name = "sentry.dotnet.godot";
			scope.Sdk.Version = NativeBridge.GetSdkVersion();
		});
	}
}
