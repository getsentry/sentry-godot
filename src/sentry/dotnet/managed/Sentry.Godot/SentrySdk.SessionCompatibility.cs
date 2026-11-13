using System;
using System.Threading;
using Sentry.Godot.Interop;

namespace Sentry.Godot;

public static partial class SentrySdk
{
    private const string PauseSessionWarning = "Sentry: PauseSession() is unsupported and now does nothing. Previously, it did not control the sessions tracked by the SDK. This method will be removed in v3.";
    private const string ResumeSessionWarning = "Sentry: ResumeSession() is unsupported and now does nothing. Previously, it did not control the sessions tracked by the SDK. This method will be removed in v3.";
    private const string CaptureSessionWarning = "Sentry: CaptureSession() now does nothing. Previously, it sent separate .NET session updates that could double-count sessions. Use StartSession() and EndSession() instead. This method will be removed in v3.";
    private const string IsSessionActiveWarning = "Sentry: IsSessionActive always returns false. It never reported the sessions tracked by the SDK. Remove checks against this property; it will be removed in v3.";
    private const string EndSessionStatusWarning = "Sentry: EndSession(status) now ends the session tracked by the SDK and ignores the status argument. Use EndSession() instead; this overload will be removed in v3.";

    private static int _pauseSessionWarningPrinted;
    private static int _resumeSessionWarningPrinted;
    private static int _captureSessionWarningPrinted;
    private static int _isSessionActiveWarningPrinted;
    private static int _endSessionStatusWarningPrinted;

    /// <remarks>
    /// Pausing sessions is unsupported and this method now does nothing.
    /// Previously, it did not control the sessions tracked by the SDK.
    /// This method will be removed in v3.
    /// </remarks>
    [Obsolete(PauseSessionWarning)]
    public static void PauseSession() => WarnUnsupportedSessionApiOnce(ref _pauseSessionWarningPrinted, PauseSessionWarning);

    /// <remarks>
    /// Resuming sessions is unsupported and this method now does nothing.
    /// Previously, it did not control the sessions tracked by the SDK.
    /// This method will be removed in v3.
    /// </remarks>
    [Obsolete(ResumeSessionWarning)]
    public static void ResumeSession() => WarnUnsupportedSessionApiOnce(ref _resumeSessionWarningPrinted, ResumeSessionWarning);

    /// <remarks>
    /// This method now does nothing. Previously, it sent separate .NET session updates that could double-count sessions.
    /// Use <see cref="StartSession()"/> and <see cref="EndSession()"/> instead.
    /// This method will be removed in v3.
    /// </remarks>
    [Obsolete(CaptureSessionWarning)]
    public static void CaptureSession(SessionUpdate sessionUpdate) =>
        WarnUnsupportedSessionApiOnce(ref _captureSessionWarningPrinted, CaptureSessionWarning);

    /// <remarks>
    /// Always returns <see langword="false"/>. It never reported the sessions tracked by the SDK.
    /// Remove checks against this property; it will be removed in v3.
    /// </remarks>
    [Obsolete(IsSessionActiveWarning)]
    public static bool IsSessionActive
    {
        get
        {
            WarnUnsupportedSessionApiOnce(ref _isSessionActiveWarningPrinted, IsSessionActiveWarning);
            return false;
        }
    }

    /// <remarks>
    /// Now ends the session tracked by the SDK and ignores the status argument.
    /// Use <see cref="EndSession()"/> instead; this overload will be removed in v3.
    /// </remarks>
    [Obsolete(EndSessionStatusWarning)]
    public static void EndSession(SessionEndStatus status = SessionEndStatus.Exited)
    {
        WarnUnsupportedSessionApiOnce(ref _endSessionStatusWarningPrinted, EndSessionStatusWarning);
        EndSession();
    }

    private static void WarnUnsupportedSessionApiOnce(ref int printed, string message)
    {
        if (Interlocked.Exchange(ref printed, 1) == 0)
        {
            NativeBridge.PushWarning(message);
        }
    }
}
