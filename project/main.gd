extends Node2D

@onready var message_edit: LineEdit = %MessageEdit
@onready var level_choice: MenuButton = %LevelChoice
@onready var breadcrumb_message: LineEdit = %BreadcrumbMessage
@onready var breadcrumb_category: LineEdit = %BreadcrumbCategory
@onready var tag_key: LineEdit = $VBoxContainer/Tags/TagKey
@onready var tag_value: LineEdit = $VBoxContainer/Tags/TagValue
@onready var context_name: LineEdit = $VBoxContainer/Context/ContextName
@onready var context_expression: CodeEdit = $VBoxContainer/Context/ContextExpression

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


func _on_set_context_pressed() -> void:
	if context_name.text.is_empty():
		print("Please provide a name for the context.")
		return

	# Filter out comments because Expression doesn't support them.
	var expr_lines := Array(context_expression.text.split("\n")).filter(
			func(s: String): return not s.begins_with("#"))
	var filtered_expression := "".join(expr_lines)

	# Parsing expression dictionary.
	var expr := Expression.new()
	var error: Error = expr.parse(filtered_expression)
	if error == OK:
		var result = expr.execute()
		if typeof(result) == TYPE_DICTIONARY:
			# Adding context.
			Sentry.set_context(context_name.text, result)
			print("Context added.")
		else:
			print("Failed set context: Dictionary is expected, but found: ", type_string(typeof(result)))
	else:
		print("Failed to parse expression: ", expr.get_error_text())
