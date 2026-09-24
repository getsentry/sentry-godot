extends CanvasLayer


func _ready() -> void:
	# Add .NET actions if editor has support for .NET
	if ClassDB.class_exists("CSharpScript"):
		%Capture.add_dotnet_actions()
