extends CanvasLayer

@onready var message_edit: LineEdit = %MessageEdit
@onready var level_choice: MenuButton = %LevelChoice
@onready var breadcrumb_message: LineEdit = %BreadcrumbMessage
@onready var breadcrumb_category: LineEdit = %BreadcrumbCategory
@onready var tag_key: LineEdit = %TagKey
@onready var tag_value: LineEdit = %TagValue
@onready var context_name: LineEdit = %ContextName
@onready var context_expression: CodeEdit = %ContextExpression
@onready var user_id: LineEdit = %UserID
@onready var username: LineEdit = %Username
@onready var email: LineEdit = %Email
@onready var infer_ip: CheckBox = %InferIP

var _event_level: Sentry.Level


func _ready() -> void:
	level_choice.get_popup().id_pressed.connect(_on_level_choice_id_pressed)
	_init_level_choice_popup()
<< << << < HEAD
	_update_user_info()
== == == =
	set_process(false)


func _process(delta: float) -> void:
	for i in range(20):
		print("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.")
>> >> >> > bfe737b(Demo: Buttonstogenerateerrors)


func _init_level_choice_popup() -> void:
	var popup: PopupMenu = level_choice.get_popup()
	popup.add_item("DEBUG", Sentry.LEVEL_DEBUG + 1)
	popup.add_item("INFO", Sentry.LEVEL_INFO + 1)
	popup.add_item("WARNING", Sentry.LEVEL_WARNING + 1)
	popup.add_item("ERROR", Sentry.LEVEL_ERROR + 1)
	popup.add_item("FATAL", Sentry.LEVEL_FATAL + 1)

	_on_level_choice_id_pressed(Sentry.LEVEL_INFO + 1)


func _update_user_info() -> void:
	# The user info is persisted in the user data directory (referenced by "user://"),
	# so it will be loaded again on subsequent launches.
	var user: SentryUser = Sentry.get_user()
	username.text = user.username
	email.text = user.email
	user_id.text = user.id


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
	print("Breadcrumb added.")


func _on_add_tag_button_pressed() -> void:
	Sentry.set_tag(tag_key.text, tag_value.text)
	if not tag_key.text.is_empty():
		print("Tag added.")


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


func _on_set_user_button_pressed() -> void:
	print("Setting user info...")
	var sentry_user := SentryUser.new()
	sentry_user.id = user_id.text
	sentry_user.username = username.text
	sentry_user.email = email.text
	if infer_ip.button_pressed:
		sentry_user.infer_ip_address()
	Sentry.set_user(sentry_user)
	print("   ", sentry_user)
	_update_user_info()


func _on_toggle_flood_printing_pressed() -> void:
	set_process(not is_processing())


func _on_gen_script_error_pressed() -> void:
	# Generate script error (type error)
	print("Generating GDScript type error...")
	var value = "123"
	var int_value: int = value
	print("Int value: ", int_value) # not executed


func _on_gen_native_error_pressed() -> void:
	# Generate native Godot error (in C++ unit)
	print("Generating C++ error...")
	load("res://file_does_not_exist")
