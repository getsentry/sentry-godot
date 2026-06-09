using System;
using Sentry.Godot.Interop;

namespace Sentry.Godot;

/// <summary>
/// Represents a native Godot event passed to the <see cref="SentryGodotOptions.BeforeSendGodot"/> callback.
/// </summary>
/// <remarks>
/// This instance is valid only while the callback is running and must not be retained.
/// </remarks>
public sealed class SentryNativeEvent : ISentryGodotEvent
{
    private readonly IntPtr _handle;

    internal SentryNativeEvent(IntPtr handle)
    {
        _handle = handle;
    }

    /// <inheritdoc/>
    public string? Id => NativeBridge.EventGetId(_handle);

    /// <inheritdoc/>
    public string? Platform => NativeBridge.EventGetPlatform(_handle);

    /// <inheritdoc/>
    public string? Message
    {
        get => NativeBridge.EventGetMessage(_handle);
        set => NativeBridge.EventSetMessage(_handle, value ?? "");
    }

    /// <inheritdoc/>
    public SentryLevel? Level
    {
        get => (SentryLevel)NativeBridge.EventGetLevel(_handle);
        set
        {
            if (value.HasValue)
            {
                NativeBridge.EventSetLevel(_handle, (int)value.Value);
            }
        }
    }

    /// <inheritdoc/>
    public string? Logger
    {
        get => NativeBridge.EventGetLogger(_handle);
        set => NativeBridge.EventSetLogger(_handle, value ?? "");
    }

    /// <inheritdoc/>
    public string? Release
    {
        get => NativeBridge.EventGetRelease(_handle);
        set => NativeBridge.EventSetRelease(_handle, value ?? "");
    }

    /// <inheritdoc/>
    public string? Distribution
    {
        get => NativeBridge.EventGetDist(_handle);
        set => NativeBridge.EventSetDist(_handle, value ?? "");
    }

    /// <inheritdoc/>
    public string? Environment
    {
        get => NativeBridge.EventGetEnvironment(_handle);
        set => NativeBridge.EventSetEnvironment(_handle, value ?? "");
    }

    /// <inheritdoc/>
    public string? GetTag(string key)
    {
        return NativeBridge.EventGetTag(_handle, key);
    }

    /// <inheritdoc/>
    public void SetTag(string key, string value)
    {
        NativeBridge.EventSetTag(_handle, key, value);
    }

    /// <inheritdoc/>
    public void UnsetTag(string key)
    {
        NativeBridge.EventRemoveTag(_handle, key);
    }

    /// <inheritdoc/>
    public int ExceptionCount => NativeBridge.EventGetExceptionCount(_handle);

    /// <inheritdoc/>
    public string? GetExceptionValue(int index)
    {
        return NativeBridge.EventGetExceptionValue(_handle, index);
    }

    /// <inheritdoc/>
    public void SetExceptionValue(int index, string value)
    {
        NativeBridge.EventSetExceptionValue(_handle, index, value);
    }
}
