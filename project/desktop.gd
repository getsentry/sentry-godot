extends CanvasLayer


func _ready() -> void:
	# Add .NET actions if editor has support for .NET
	if ClassDB.class_exists("CSharpScript"):
		var dotnet_scene: PackedScene = load("res://views/dotnet_actions.tscn")
		%MoreActions.add_child(dotnet_scene.instantiate())
