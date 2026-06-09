namespace Sentry.Godot;

/// <summary>
/// Provides a user-facing, mutable view of an event passed to SentryGodotOptions.BeforeSendGodot.
/// </summary>
/// <remarks>
/// This callback receives both managed events, wrapped as SentryManagedEvent, and native
/// GDScript or engine events, wrapped as SentryNativeEvent. Members expose only the data
/// both layers can support; implementation-specific gaps are documented by each wrapper.
/// Native crash events are not routed through this callback.
/// </remarks>
public interface ISentryGodotEvent
{
    /// <summary>
    /// The event's unique identifier.
    /// </summary>
    string? Id { get; }

    /// <summary>
    /// The platform that produced the event.
    /// </summary>
    string? Platform { get; }

    /// <summary>
    /// The message that describes the event.
    /// </summary>
    string? Message { get; set; }

    /// <summary>
    /// The severity of the event.
    /// </summary>
    SentryLevel? Level { get; set; }

    /// <summary>
    /// The name of the logger that captured the event.
    /// </summary>
    string? Logger { get; set; }

    /// <summary>
    /// The release version of the application.
    /// </summary>
    string? Release { get; set; }

    /// <summary>
    /// The distribution of the application.
    /// </summary>
    string? Distribution { get; set; }

    /// <summary>
    /// The environment the event was captured in.
    /// </summary>
    string? Environment { get; set; }

    /// <summary>
    /// Returns the value of the tag with the given key, or null if the tag is not set.
    /// </summary>
    string? GetTag(string key);

    /// <summary>
    /// Sets the tag with the given key to the given value.
    /// </summary>
    void SetTag(string key, string value);

    /// <summary>
    /// Removes the tag with the given key.
    /// </summary>
    void UnsetTag(string key);

    /// <summary>
    /// The number of exceptions attached to the event.
    /// </summary>
    int ExceptionCount { get; }

    /// <summary>
    /// Returns the message of the exception at the given index, or null if the index is out of range.
    /// </summary>
    string? GetExceptionValue(int index);

    /// <summary>
    /// Sets the message of the exception at the given index.
    /// </summary>
    void SetExceptionValue(int index, string value);

    // Gaps (not bridged yet): fingerprint, user, extra data, breadcrumbs, contexts, and
    // stack-frame variables are intentionally left out of the unified surface. Several of
    // these have no accessor on the native SentryEvent yet (for example, fingerprint isn't
    // reachable from GDScript), so they are omitted until both layers can back them rather
    // than exposing members that only work on one side.
    // See: https://github.com/getsentry/sentry-godot/issues/706
}
