using Godot;
using System;
using Sentry.Godot;

public partial class ExampleDotnet : Node2D {
	public override void _Ready() {
		GD.Print("[Sentry.NET] Initializing...");
		NativeBridge.SetTag("biome", "jungle");
	}
}
