using Sentry.Extensibility;

namespace Sentry.Godot.Internal;

internal class DefaultAttachmentsProcessor : ISentryEventProcessor
{
    public SentryEvent? Process(SentryEvent @event)
    {
        Sentry.Godot.Interop.NativeBridge.ProcessDefaultAttachments();
        return @event;
    }
}
