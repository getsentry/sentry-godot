using Sentry.Extensibility;

namespace Sentry.Godot.Internal;

/// <summary>
/// Invokes the user-configured BeforeSendGodot callback for managed events.
/// </summary>
/// <remarks>
/// Managed events are passed to the callback as <see cref="SentryManagedEvent" /> instances,
/// allowing the event to be modified before it is sent. Return the event to keep it, or return
/// null to discard it. The same callback is also used for engine and GDScript events, providing
/// a single place to filter all Godot events.
/// </remarks>
internal sealed class GodotBeforeSendProcessor : ISentryEventProcessor
{
    public SentryEvent? Process(SentryEvent @event)
    {
        var callback = Sentry.Godot.SentrySdk.CurrentOptions?.BeforeSendGodot;
        if (callback is null)
        {
            return @event;
        }

        var result = callback(new SentryManagedEvent(@event));

        return result is null ? null : @event;
    }
}
