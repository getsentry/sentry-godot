using System.Collections.Generic;
using Sentry.Extensibility;
using Sentry.Godot.Interop;

namespace Sentry.Godot.Internal;

/// <summary>
/// Attaches Mach-O debug images to events on iOS so NativeAOT stack frames symbolicate server-side.
/// </summary>
/// <remarks>
/// Frames arrive with <c>image_addr</c> set but no matching <c>debug_meta.images</c> entries,
/// leaving the symbolicator with nothing to bind to. This processor reads the images from
/// the in-process sentry-cocoa cache and appends them to the event.
/// </remarks>
internal sealed class CocoaDebugImagesProcessor : ISentryEventProcessor
{
    public SentryEvent? Process(SentryEvent @event)
    {
        var addresses = CollectImageAddresses(@event);
        if (addresses.Count == 0)
        {
            return @event;
        }

        var images = NativeBridge.GetCocoaDebugImages(addresses);
        if (images.Count == 0)
        {
            return @event;
        }

        @event.DebugImages ??= [];

        // Remember existing images to avoid adding duplicates.
        var existing = new HashSet<long>();
        foreach (var img in @event.DebugImages)
        {
            if (img.ImageAddress is long addr)
            {
                existing.Add(addr);
            }
        }

        foreach (var img in images)
        {
            if (img.ImageAddress is long addr && existing.Add(addr))
            {
                @event.DebugImages.Add(img);
            }
        }

        return @event;
    }

    private static HashSet<long> CollectImageAddresses(SentryEvent @event)
    {
        var set = new HashSet<long>();

        var exceptions = @event.SentryExceptions;
        if (exceptions is not null)
        {
            foreach (var ex in exceptions)
            {
                AddFramesFrom(ex.Stacktrace, set);
            }
        }

        var threads = @event.SentryThreads;
        if (threads is not null)
        {
            foreach (var th in threads)
            {
                AddFramesFrom(th.Stacktrace, set);
            }
        }
        return set;
    }

    private static void AddFramesFrom(SentryStackTrace? trace, HashSet<long> set)
    {
        if (trace is null)
        {
            return;
        }
        foreach (var frame in trace.Frames)
        {
            if (frame.ImageAddress is long addr && addr != 0)
            {
                set.Add(addr);
            }
        }
    }
}
