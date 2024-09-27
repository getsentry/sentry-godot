extends Node2D

@onready var message_edit: LineEdit = %MessageEdit

func _on_capture_button_pressed() -> void:
	Sentry.capture_message_event(message_edit.text)
