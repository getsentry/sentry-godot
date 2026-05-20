using Sentry.Extensibility;

namespace Sentry.Godot.Internal;

// <summary>
// Adds default attachments (log, screenshot, and view hierarchy) for .NET events.
// </summary>
internal class DefaultAttachmentsProcessor : ISentryEventProcessorWithHint
{
    public SentryEvent? Process(SentryEvent @event)
    {
        return @event;
    }

    public SentryEvent? Process(SentryEvent @event, SentryHint hint)
    {
        Sentry.Godot.Interop.NativeBridge.ProcessDefaultAttachments(@event.Level ?? SentryLevel.Error);
        foreach (SentryAttachment att in Sentry.Godot.SentrySdk.CurrentOptions!.DefaultAttachments)
        {
            hint.Attachments.Add(att);
        }
        return @event;
    }
}
