extends CanvasLayer

const MOBILE_TESTS_FILE := "res://test/mobile_tests.gd"


func _ready() -> void:
	get_viewport().size = Vector2(810, 1530) # simulate mobile screen on desktop
	get_viewport().get_window().content_scale_factor = 3.0
	get_viewport().get_window().content_scale_mode = Window.CONTENT_SCALE_MODE_CANVAS_ITEMS
	get_viewport().get_window().content_scale_aspect = Window.CONTENT_SCALE_ASPECT_EXPAND

	# Show tests only if the test file is available
	%Tests.visible = FileAccess.file_exists(MOBILE_TESTS_FILE)


func _on_run_tests_button_pressed() -> void:
	var tests = load(MOBILE_TESTS_FILE).new()
	tests.run_tests()
