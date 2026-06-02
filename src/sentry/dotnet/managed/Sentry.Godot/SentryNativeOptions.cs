using System;

namespace Sentry.Godot;

/// <summary>
/// Contains options for the native layer, accessed through
/// <see cref="SentryGodotOptions.Native"/>.
/// </summary>
/// <remarks>
/// The native layer handles events that originate outside .NET, such as GDScript and engine errors.
/// </remarks>
public sealed class SentryNativeOptions
{
    internal Func<SentryNativeEvent, SentryNativeEvent?>? BeforeSend { get; private set; }

    /// <summary>
    /// Sets the callback invoked before a native event is sent, allowing you to inspect, modify, or discard it.
    /// </summary>
    /// <remarks>
    /// Native events include GDScript and engine errors. The callback receives a
    /// <see cref="SentryNativeEvent"/>; return it to send the event, or return null to discard it.
    /// Native crash events are not passed to this callback.
    /// Managed (.NET) events are configured through
    /// <see cref="SentryOptions.SetBeforeSend(Func{SentryEvent, SentryHint, SentryEvent?})"/> instead.
    /// </remarks>
    public void SetBeforeSend(Func<SentryNativeEvent, SentryNativeEvent?> beforeSend)
    {
        BeforeSend = beforeSend;
    }
}
