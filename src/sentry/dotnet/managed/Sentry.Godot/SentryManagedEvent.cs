using System.Linq;

namespace Sentry.Godot;

/// <summary>
/// Wraps a managed Sentry event for use with the unified BeforeSendGodot callback.
/// </summary>
/// <remarks>
/// Changes made through this wrapper update the underlying event.
/// Use <see cref="Inner"/> to access managed-only details such as fingerprint, user, and contexts.
/// </remarks>
public sealed class SentryManagedEvent : ISentryGodotEvent
{
    /// <summary>
    /// The wrapped managed event. Use it to access properties that are not part of the unified interface.
    /// </summary>
    public SentryEvent Inner { get; }

    internal SentryManagedEvent(SentryEvent inner)
    {
        Inner = inner;
    }

    /// <inheritdoc/>
    public string? Id => Inner.EventId.ToString();

    /// <inheritdoc/>
    public string? Platform => Inner.Platform;

    /// <inheritdoc/>
    public string? Message
    {
        get => Inner.Message?.Formatted ?? Inner.Message?.Message;
        set => Inner.Message = value;
    }

    /// <inheritdoc/>
    public SentryLevel? Level { get => Inner.Level; set => Inner.Level = value; }

    /// <inheritdoc/>
    public string? Logger { get => Inner.Logger; set => Inner.Logger = value; }

    /// <inheritdoc/>
    public string? Release { get => Inner.Release; set => Inner.Release = value; }

    /// <inheritdoc/>
    public string? Distribution { get => Inner.Distribution; set => Inner.Distribution = value; }

    /// <inheritdoc/>
    public string? Environment { get => Inner.Environment; set => Inner.Environment = value; }

    /// <inheritdoc/>
    public string? GetTag(string key) => Inner.Tags.TryGetValue(key, out var value) ? value : null;

    /// <inheritdoc/>
    public void SetTag(string key, string value) => Inner.SetTag(key, value);

    /// <inheritdoc/>
    public void UnsetTag(string key) => Inner.UnsetTag(key);

    /// <inheritdoc/>
    public int ExceptionCount => Inner.SentryExceptions?.Count() ?? 0;

    /// <inheritdoc/>
    public string? GetExceptionValue(int index) => Inner.SentryExceptions?.ElementAtOrDefault(index)?.Value;

    /// <inheritdoc/>
    public void SetExceptionValue(int index, string value)
    {
        var exception = Inner.SentryExceptions?.ElementAtOrDefault(index);
        if (exception is not null)
        {
            exception.Value = value;
        }
    }
}
