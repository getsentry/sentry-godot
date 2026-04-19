using Godot;
using System;
using System.Threading;

public partial class DotnetActions : VBoxContainer
{
    public void TriggerException()
    {
        GD.Print("Triggering exception...");
        throw new Exception("Test CSharp exceptions");
    }

    public void TriggerThreadException()
    {
        GD.Print("Triggering exception from thread...");
        var thread = new Thread(() => throw new Exception("Test CSharp thread exception"));
        thread.Start();
        thread.Join();
    }

    public void TriggerHandledException()
    {
        GD.Print("Triggering handled exception...");
        try
        {
            throw new InvalidOperationException("This is handled by user code and should not be captured");
        }
        catch (Exception e)
        {
            GD.Print($"User caught: {e.Message}");
        }
    }

    public void TriggerRethrow()
    {
        GD.Print("Triggering rethrow...");
        try
        {
            throw new Exception("Rethrow with throw;");
        }
        catch
        {
            throw;
        }
    }

    public void TriggerWrappedRethrow()
    {
        GD.Print("Triggering wrapped rethrow...");
        try
        {
            throw new Exception("Inner exception");
        }
        catch (Exception ex)
        {
            throw new InvalidOperationException("Wrapped rethrow", ex);
        }
    }

    // Regression test for listener-lag correlation: an exception is replaced by
    // another thrown from a finally block, swallowed by user code, and then a
    // real exception is thrown that should reach the bridge. The bridge must
    // report "Real bridge exception", not the earlier swallowed exception.
    public void TriggerNestedException()
    {
        GD.Print("Triggering nested finally-replaced exception...");
        try
        {
            try
            {
                throw new InvalidOperationException("Replaced in finally");
            }
            finally
            {
                throw new ArgumentException("Replaces in-flight exception");
            }
        }
        catch (ArgumentException)
        {
        }

        throw new Exception("Real bridge exception");
    }

    public void TriggerReplacedThenRethrown()
    {
        GD.Print("Triggering replaced-then-rethrown exception...");

        try
        {
            try { throw new Exception("X (should NOT be captured)"); }
            finally { throw new Exception("Y (should be captured)"); }   // Y replaces X
        }
        catch { throw; }       // rethrow Y
    }
}
