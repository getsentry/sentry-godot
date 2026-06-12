using System;
using Sentry.Godot.Interop;

namespace Sentry.Godot;

/// <summary>
/// Represents a native Godot event passed to the before-send callback registered
/// through <see cref="SentryNativeOptions.SetBeforeSend"/>.
/// </summary>
/// <remarks>
/// This instance is valid only while the callback is running and must not be retained.
/// Each member reads from or writes to the live native event across the interop boundary,
/// so only the fields a callback touches cross it.
/// </remarks>
public sealed class SentryNativeEvent
{
    private readonly IntPtr _handle;

    internal SentryNativeEvent(IntPtr handle)
    {
        _handle = handle;
    }

    /// <summary>
    /// The event's unique identifier.
    /// </summary>
    public string? Id => NativeBridge.EventGetId(_handle);

    /// <summary>
    /// The platform that produced the event.
    /// </summary>
    public string? Platform => NativeBridge.EventGetPlatform(_handle);

    /// <summary>
    /// The message that describes the event.
    /// </summary>
    public string? Message
    {
        get => NativeBridge.EventGetMessage(_handle);
        set => NativeBridge.EventSetMessage(_handle, value ?? "");
    }

    /// <summary>
    /// The severity of the event.
    /// </summary>
    public SentryLevel Level
    {
        get => (SentryLevel)NativeBridge.EventGetLevel(_handle);
        set => NativeBridge.EventSetLevel(_handle, (int)value);
    }

    /// <summary>
    /// The name of the logger that captured the event.
    /// </summary>
    public string? Logger
    {
        get => NativeBridge.EventGetLogger(_handle);
        set => NativeBridge.EventSetLogger(_handle, value ?? "");
    }

    /// <summary>
    /// The release version of the application.
    /// </summary>
    public string? Release
    {
        get => NativeBridge.EventGetRelease(_handle);
        set => NativeBridge.EventSetRelease(_handle, value ?? "");
    }

    /// <summary>
    /// The distribution of the application.
    /// </summary>
    public string? Distribution
    {
        get => NativeBridge.EventGetDist(_handle);
        set => NativeBridge.EventSetDist(_handle, value ?? "");
    }

    /// <summary>
    /// The environment the event was captured in.
    /// </summary>
    public string? Environment
    {
        get => NativeBridge.EventGetEnvironment(_handle);
        set => NativeBridge.EventSetEnvironment(_handle, value ?? "");
    }

    /// <summary>
    /// Returns the value of the tag with the given key, or null if the tag is not set.
    /// </summary>
    public string? GetTag(string key)
    {
        return NativeBridge.EventGetTag(_handle, key);
    }

    /// <summary>
    /// Sets the tag with the given key to the given value.
    /// </summary>
    public void SetTag(string key, string value)
    {
        NativeBridge.EventSetTag(_handle, key, value);
    }

    /// <summary>
    /// Removes the tag with the given key.
    /// </summary>
    public void UnsetTag(string key)
    {
        NativeBridge.EventRemoveTag(_handle, key);
    }

    /// <summary>
    /// The number of exceptions attached to the event.
    /// </summary>
    public int ExceptionCount => NativeBridge.EventGetExceptionCount(_handle);

    /// <summary>
    /// Returns the message of the exception at the given index, or null if the index is out of range.
    /// </summary>
    public string? GetExceptionValue(int index)
    {
        return NativeBridge.EventGetExceptionValue(_handle, index);
    }

    /// <summary>
    /// Sets the message of the exception at the given index.
    /// </summary>
    public void SetExceptionValue(int index, string value)
    {
        NativeBridge.EventSetExceptionValue(_handle, index, value);
    }
}
