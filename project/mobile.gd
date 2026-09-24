extends CanvasLayer

@onready var output_pane: VBoxContainer = %OutputPane
@onready var output_button: Button = %OutputButton


func _ready() -> void:
	get_viewport().size = Vector2(810, 1530) # simulate mobile screen on desktop
	get_viewport().get_window().content_scale_factor = 3.0
	get_viewport().get_window().content_scale_mode = Window.CONTENT_SCALE_MODE_CANVAS_ITEMS
	get_viewport().get_window().content_scale_aspect = Window.CONTENT_SCALE_ASPECT_EXPAND

	# Add .NET actions if editor has support for .NET
	if ClassDB.class_exists("CSharpScript"):
		%Capture.add_dotnet_actions()


func _on_output_button_toggled(button_pressed: bool) -> void:
	output_pane.visible = button_pressed
	output_button.text = "OUTPUT ▼" if button_pressed else "OUTPUT ▲"
