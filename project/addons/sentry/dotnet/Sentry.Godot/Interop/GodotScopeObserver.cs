using System;
using Sentry;

namespace Sentry.Godot.Interop;

/// <summary>
/// Scope Observer to sync changes to native layer.
/// </summary>
internal class GodotScopeObserver : IScopeObserver
{
    [ThreadStatic]
    private static bool _syncing;

    public void AddBreadcrumb(Breadcrumb breadcrumb)
    {
        if (_syncing)
        {
            return;
        }
        _syncing = true;
        try
        {
            NativeBridge.AddBreadcrumb(breadcrumb);
        }
        finally
        {
            _syncing = false;
        }
    }

    public void SetExtra(string key, object? value)
    {
        // NOTE: Godot SDK doesn't support extras.
    }

    public void SetTag(string key, string value)
    {
        if (_syncing)
        {
            return;
        }
        _syncing = true;
        try
        {
            NativeBridge.SetTag(key, value);
        }
        finally
        {
            _syncing = false;
        }
    }

    public void UnsetTag(string key)
    {
        if (_syncing)
        {
            return;
        }
        _syncing = true;
        try
        {
            NativeBridge.RemoveTag(key);
        }
        finally
        {
            _syncing = false;
        }
    }

    public void SetUser(SentryUser? user)
    {
        if (_syncing)
        {
            return;
        }
        _syncing = true;
        try
        {
            NativeBridge.SetUser(user);
        }
        finally
        {
            _syncing = false;
        }
    }

    public void SetTrace(SentryId traceId, SpanId parentSpanId)
    {
        // Trace context is owned by the native layer and shared with .NET at init
        // via NativeBridge.GetTraceContext(). Scope-level trace changes are not
        // propagated back to native.
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
