using Godot;
using System;
using System.Threading;

public partial class DotnetActions : VBoxContainer
{
    public void TriggerException()
    {
        GD.Print("Triggering exception...");
        throw new Exception("Exception (should be captured)");
    }

    public void TriggerThreadException()
    {
        GD.Print("Triggering thread exception (likely to crash)...");
        var thread = new Thread(() => throw new Exception("Thread exception (should be captured)"));
        thread.Start();
        thread.Join();
    }

    public void TriggerUserHandled()
    {
        GD.Print("Triggering user-handled exception...");
        try
        {
            throw new InvalidOperationException("User-handled (should NOT be captured)");
        }
        catch
        {
        }
    }

    public void TriggerBareRethrow()
    {
        GD.Print("Triggering bare rethrow...");
        try
        {
            throw new Exception("Bare rethrow (should be captured)");
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
            throw new Exception("Inner (should NOT be captured)");
        }
        catch (Exception ex)
        {
            throw new InvalidOperationException("Wrapped (should be captured)", ex);
        }
    }

    public void TriggerNestedFinally()
    {
        GD.Print("Triggering nested finally throw...");
        try
        {
            try
            {
                throw new InvalidOperationException("Replaced in finally (should NOT be captured)");
            }
            finally
            {
                throw new ArgumentException("Thrown from finally, user-caught (should NOT be captured)");
            }
        }
        catch (ArgumentException)
        {
        }

        throw new Exception("Post-catch throw (should be captured)");
    }

    public void TriggerReplacedThenRethrown()
    {
        GD.Print("Triggering replaced-then-rethrown...");

        try
        {
            try { throw new Exception("Replaced in finally (should NOT be captured)"); }
            finally { throw new Exception("Thrown from finally, rethrown (should be captured)"); }
        }
        catch { throw; }
    }
}
