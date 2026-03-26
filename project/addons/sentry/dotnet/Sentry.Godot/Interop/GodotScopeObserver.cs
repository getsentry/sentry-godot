using System;
using Sentry;

namespace Sentry.Godot.Interop;

/// <summary>
/// Scope Observer to sync changes to native layer.
/// </summary>
public class GodotScopeObserver : IScopeObserver {
	public void AddBreadcrumb(Breadcrumb breadcrumb) {}

	public void SetExtra(string key, object? value) {}

	public void SetTag(string key, string value) {}

	public void UnsetTag(string key) {}

	public void SetUser(SentryUser? user) {}

	public void SetTrace(SentryId traceId, SpanId parentSpanId) =>
			NativeBridge.SetTrace(traceId.ToString(), parentSpanId.ToString());
}
