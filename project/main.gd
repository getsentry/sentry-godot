extends Node2D

@onready var message_edit: LineEdit = %MessageEdit
@onready var level_choice: MenuButton = %LevelChoice

var _event_level: Sentry.Level


func _ready() -> void:
	level_choice.get_popup().id_pressed.connect(_on_level_choice_id_pressed)
	_init_level_choice_popup()


func _init_level_choice_popup() -> void:
	var popup: PopupMenu = level_choice.get_popup()
	popup.add_item("DEBUG", Sentry.LEVEL_DEBUG + 1)
	popup.add_item("INFO", Sentry.LEVEL_INFO + 1)
	popup.add_item("WARNING", Sentry.LEVEL_WARNING + 1)
	popup.add_item("ERROR", Sentry.LEVEL_ERROR + 1)
	popup.add_item("FATAL", Sentry.LEVEL_FATAL + 1)

	_on_level_choice_id_pressed(Sentry.LEVEL_INFO + 1)


func _on_level_choice_id_pressed(id: int) -> void:
	_event_level = (id - 1) as Sentry.Level
	match _event_level:
		Sentry.LEVEL_DEBUG:
			level_choice.text = "DEBUG"
		Sentry.LEVEL_INFO:
			level_choice.text = "INFO"
		Sentry.LEVEL_WARNING:
			level_choice.text = "WARNING"
		Sentry.LEVEL_ERROR:
			level_choice.text = "ERROR"
		Sentry.LEVEL_FATAL:
			level_choice.text = "FATAL"


func _on_capture_button_pressed() -> void:
	Sentry.capture_message_event(message_edit.text, _event_level)
