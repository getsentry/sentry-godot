extends Node2D

@onready var message_edit: LineEdit = %MessageEdit
@onready var level_choice: MenuButton = %LevelChoice
@onready var breadcrumb_message: LineEdit = %BreadcrumbMessage
@onready var breadcrumb_category: LineEdit = %BreadcrumbCategory
@onready var tag_key: LineEdit = $VBoxContainer/Tags/TagKey
@onready var tag_value: LineEdit = $VBoxContainer/Tags/TagValue

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
	Sentry.capture_message(message_edit.text, _event_level)
	print("Captured message event. Event ID: " + Sentry.get_last_event_id())


func _on_add_breadcrumb_button_pressed() -> void:
	Sentry.add_breadcrumb(breadcrumb_message.text, breadcrumb_category.text, Sentry.LEVEL_ERROR, "default")


func _on_add_tag_button_pressed() -> void:
	Sentry.set_tag(tag_key.text, tag_value.text)


func _on_crash_button_pressed() -> void:
	OS.crash("Crashing on button press")
