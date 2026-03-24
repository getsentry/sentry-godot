using Godot;
using System;
using System.Threading;
using Sentry.Godot;

public partial class ExampleDotnet : CanvasLayer {
	public override void _Ready() {
		SentrySdk.Init();
	}

	public void TriggerException() {
		throw new Exception("Test CSharp exceptions");
	}

	public void TriggerThreadException() {
		var thread = new Thread(() => throw new Exception("Test CSharp thread exception"));
		thread.Start();
		thread.Join();
	}

	public void TriggerHandledException() {
		try {
			throw new InvalidOperationException("This is handled by user code");
		} catch (Exception e) {
			GD.Print($"User caught: {e.Message}");
		}
	}

	public void TriggerRethrow() {
		try {
			throw new Exception("Rethrow with throw;");
		} catch {
			throw;
		}
	}

	public void TriggerWrappedRethrow() {
		try {
			throw new Exception("Inner exception");
		} catch (Exception ex) {
			throw new InvalidOperationException("Wrapped rethrow", ex);
		}
	}
}
