using System;
using Sentry;

namespace Sentry.Godot.Interop;

/// <summary>
/// Synchronizes scope changes to native layer.
/// </summary>
/// <remarks>
/// Local scope changes are NOT synced, which prevents them from leaking into the native current scope.
/// </remarks>
internal class GodotScopeObserver : IScopeObserver
{
    // Prevent feedback loops when syncing scope changes across layers.
    // Incremented here for outbound updates and by native callers in NativeBridge for inbound updates.
    [ThreadStatic] private static uint _syncDepth;
    internal static bool IsSyncing => _syncDepth > 0;

    internal readonly struct SyncGuard : IDisposable
    {
        public SyncGuard() { ++_syncDepth; }
        public void Dispose() => --_syncDepth;
    }

    public void AddBreadcrumb(Breadcrumb breadcrumb)
    {
        if (IsSyncing || SentrySdk.InLocalScope)
        {
            return;
        }
        using var _ = new SyncGuard();
        NativeBridge.AddBreadcrumb(breadcrumb);
    }

    public void SetExtra(string key, object? value)
    {
        // NOTE: Godot SDK doesn't support extras.
    }

    public void SetTag(string key, string value)
    {
        if (IsSyncing || SentrySdk.InLocalScope)
        {
            return;
        }
        using var _ = new SyncGuard();
        NativeBridge.SetTag(key, value);
    }

    public void UnsetTag(string key)
    {
        if (IsSyncing || SentrySdk.InLocalScope)
        {
            return;
        }
        using var _ = new SyncGuard();
        NativeBridge.RemoveTag(key);
    }

    public void SetUser(SentryUser? user)
    {
        if (IsSyncing || SentrySdk.InLocalScope)
        {
            return;
        }
        using var _ = new SyncGuard();
        NativeBridge.SetUser(user);
    }

    public void SetTrace(SentryId traceId, SpanId parentSpanId)
    {
        // TODO: forward to native trace API once wired through NativeBridge.
    }

    public void AddAttachment(SentryAttachment attachment)
    {
        // TODO: forward to native attachment API once wired through NativeBridge.
    }

    public void ClearAttachments()
    {
        // TODO: forward to native attachment API once wired through NativeBridge.
    }
}
