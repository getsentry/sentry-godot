using System;

namespace Sentry.Godot;

/// <summary>
/// Contains options for the native layer, accessed through
/// <see cref="SentryGodotOptions.Native"/>.
/// </summary>
/// <remarks>
/// The native layer handles events that originate outside .NET, such as GDScript and engine errors.
/// Configure these options in Project Settings under <b>Sentry &gt; Native</b>,
/// or override them in the initialization callback passed to <see cref="SentrySdk.Init(Action{SentryGodotOptions})"/>.
/// Crashpad is the default crash backend. To use the native crash backend in an exported game,
/// select <b>Native</b> under <b>Sentry &gt; Crash Backend</b> in the Linux or Windows export preset.
/// </remarks>
public sealed class SentryNativeOptions
{
    /// <summary>
    /// Controls how crash data is collected and sent to Sentry: a minidump for server-side stackwalking,
    /// an event with client-side stacktraces, or an event with client-side stacktraces and a minidump attachment.
    /// Defaults to <see cref="SentryCrashReportingMode.NativeWithMinidump"/>.
    /// </summary>
    /// <remarks>
    /// Applies only to the native crash backend on Linux and Windows; other crash backends ignore this option.
    /// </remarks>
    public SentryCrashReportingMode CrashReportingMode { get; set; } = SentryCrashReportingMode.NativeWithMinidump;

    /// <summary>
    /// Controls how much memory is captured in crash minidumps. Defaults to <see cref="SentryMinidumpMode.Smart"/>.
    /// </summary>
    /// <remarks>
    /// Applies only when <see cref="CrashReportingMode"/> includes a minidump.
    /// Larger captures take longer to generate and upload.
    /// Applies only to the native crash backend on Linux and Windows; other crash backends ignore this option.
    /// </remarks>
    public SentryMinidumpMode MinidumpMode { get; set; } = SentryMinidumpMode.Smart;

    /// <summary>
    /// Controls which threads receive client-side stacktraces. Defaults to <see cref="SentryThreadStackwalkMode.All"/>.
    /// </summary>
    /// <remarks>
    /// Applies only when <see cref="CrashReportingMode"/> enables native stackwalking. In crashed-thread-only mode,
    /// other threads are still reported with their IDs and names, without stacktraces.
    /// Walking all threads provides more context but can delay crash collection in applications with many threads.
    /// Applies only to the native crash backend on Linux and Windows; other crash backends ignore this option.
    /// </remarks>
    public SentryThreadStackwalkMode ThreadStackwalkMode { get; set; } = SentryThreadStackwalkMode.All;

    /// <summary>
    /// Controls whether the crashed application waits for upload and daemon shutdown. Defaults to <see cref="SentryCrashUploadMode.Sync"/>.
    /// </summary>
    /// <remarks>
    /// Asynchronous upload allows the application to
    /// terminate after crash data is captured while the daemon continues uploading. Capture itself still blocks.
    /// Applies only to the native crash backend on Linux and Windows; other crash backends ignore this option.
    /// </remarks>
    public SentryCrashUploadMode CrashUploadMode { get; set; } = SentryCrashUploadMode.Sync;

    internal Func<SentryNativeEvent, SentryNativeEvent?>? BeforeSend { get; private set; }
    internal Func<SentryNativeEvent, SentryNativeEvent?>? BeforeSendTransaction { get; private set; }

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

    /// <summary>
    /// Sets the callback invoked before a native transaction is sent, allowing you to inspect, modify, or discard it.
    /// </summary>
    /// <remarks>
    /// Native transactions include performance events created in the native layer (such as from GDScript). The callback
    /// receives a <see cref="SentryNativeEvent"/>; return it to send the transaction, or return null to discard it.
    /// Transactions support common event properties such as release, distribution, environment, and tags.
    /// Error-specific properties such as message, level, logger, and exceptions are not available. Managed (.NET)
    /// transactions are configured through
    /// <see cref="SentryOptions.SetBeforeSendTransaction(Func{SentryTransaction, SentryTransaction})"/> instead.
    /// </remarks>
    public void SetBeforeSendTransaction(Func<SentryNativeEvent, SentryNativeEvent?> beforeSendTransaction)
    {
        BeforeSendTransaction = beforeSendTransaction;
    }
}

/// <summary>
/// Controls what the native crash backend captures and sends.
/// </summary>
public enum SentryCrashReportingMode
{
    /// <summary>Send a minidump for server-side stackwalking and symbolication.</summary>
    Minidump = 0,
    /// <summary>Send an event with client-side stacktraces, without a minidump.</summary>
    Native = 1,
    /// <summary>Send an event with client-side stacktraces and a minidump attachment.</summary>
    NativeWithMinidump = 2,
}

/// <summary>
/// Controls how much memory the native crash backend captures in minidumps.
/// </summary>
public enum SentryMinidumpMode
{
    /// <summary>Capture only stack memory.</summary>
    StackOnly = 0,
    /// <summary>Capture stack memory and heap memory around the crash site.</summary>
    Smart = 1,
    /// <summary>Capture full process memory.</summary>
    Full = 2,
}

/// <summary>
/// Controls which threads the native crash backend walks client-side.
/// </summary>
public enum SentryThreadStackwalkMode
{
    /// <summary>Walk only the crashed thread. Other threads retain their IDs and names.</summary>
    CrashedOnly = 1,
    /// <summary>Walk all threads.</summary>
    All = 2,
}

/// <summary>
/// Controls when the crashed application can terminate relative to upload and daemon shutdown.
/// </summary>
public enum SentryCrashUploadMode
{
    /// <summary>Keep the application blocked until the crash daemon finishes upload and shutdown.</summary>
    Sync = 0,
    /// <summary>Allow the application to terminate after crash data is captured while the daemon continues upload and shutdown work.</summary>
    Async = 1,
}
