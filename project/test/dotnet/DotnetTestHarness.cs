using Godot;
using Sentry;
using Sentry.Godot;

/// <summary>
/// Drives the .NET layer for the CPP integration testing (doctest).
/// See /tests/cpp/.
/// </summary>
public partial class DotnetTestHarness : RefCounted
{
    public void Init()
    {
        SentrySdk.Init(options =>
        {
            options.Debug = false;
            options.AttachScreenshot = false; // CI runs headless
        });
    }

    public void InitWithNativeHooks()
    {
        SentrySdk.Init(options =>
        {
            options.Debug = false;
            options.AttachScreenshot = false; // CI runs headless
            options.Native.SetBeforeSend(OnBeforeSend);
        });
    }

    public void Close()
    {
        SentrySdk.Close();
    }

    public string GetCurrentTraceId()
    {
        return SentrySdk.GetTraceHeader()?.TraceId.ToString() ?? "";
    }

    private ITransactionTracer _activeTransaction;

    public string StartTransaction(string name, string operation)
    {
        _activeTransaction = SentrySdk.StartTransaction(name, operation);
        SentrySdk.ConfigureScope(scope => scope.Transaction = _activeTransaction);
        return _activeTransaction.TraceId.ToString();
    }

    public string FinishTransaction()
    {
        _activeTransaction?.Finish();
        _activeTransaction = null;
        return GetCurrentTraceId();
    }

    private readonly Godot.Collections.Dictionary _seenEventValues = [];
    public Godot.Collections.Dictionary GetSeenEventValues() => _seenEventValues;

    /// <summary>
    /// Native before-send callback exercised by the CPP tests.
    /// Records the values it reads through the getters, then overrides them through the setters.
    /// </summary>
    private SentryNativeEvent OnBeforeSend(SentryNativeEvent ev)
    {
        if (ev.Message is not null && ev.Message.Contains("DROP_ME"))
        {
            return null;
        }

        _seenEventValues["message"] = ev.Message;
        _seenEventValues["level"] = ev.Level.ToString();
        _seenEventValues["release"] = ev.Release;
        _seenEventValues["environment"] = ev.Environment;
        _seenEventValues["logger"] = ev.Logger;
        _seenEventValues["tag"] = ev.GetTag("before_send.read_me");

        ev.Message = "Before-send override: 世界 👋";
        ev.Level = SentryLevel.Warning;
        ev.SetTag("before_send.added", "added 世界 👋");
        ev.UnsetTag("before_send.remove_me");
        return ev;
    }
}
