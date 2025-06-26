extends CanvasLayer

func _ready() -> void:
	get_viewport().size = Vector2(810, 1530) # simulate mobile screen on desktop
	get_viewport().get_window().content_scale_factor = 3.0
	get_viewport().get_window().content_scale_mode = Window.CONTENT_SCALE_MODE_CANVAS_ITEMS
	get_viewport().get_window().content_scale_aspect = Window.CONTENT_SCALE_ASPECT_EXPAND

	if OS.get_name() == "Android":
		var tests = load("res://test/test_on_android.gd").new()
		tests.run_tests()
