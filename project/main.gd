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

var _event_level: SentrySDK.Level


func _ready() -> void:
	level_choice.get_popup().id_pressed.connect(_on_level_choice_id_pressed)
	_init_level_choice_popup()
	_update_user_info()
	
	var ev := SentrySDK.create_event()
	ev.message = "manual event"
	SentrySDK.capture_event(ev)


func _init_level_choice_popup() -> void:
	var popup: PopupMenu = level_choice.get_popup()
	popup.add_item("DEBUG", SentrySDK.LEVEL_DEBUG)
	popup.add_item("INFO", SentrySDK.LEVEL_INFO)
	popup.add_item("WARNING", SentrySDK.LEVEL_WARNING)
	popup.add_item("ERROR", SentrySDK.LEVEL_ERROR)
	popup.add_item("FATAL", SentrySDK.LEVEL_FATAL)

	_on_level_choice_id_pressed(SentrySDK.LEVEL_INFO)


func _update_user_info() -> void:
	# The user info is persisted in the user data directory (referenced by "user://"),
	# so it will be loaded again on subsequent launches.
	var user: SentryUser = SentrySDK.get_user()
	username.text = user.username
	email.text = user.email
	user_id.text = user.id


func _print_info(msg: String) -> void:
	print("INFO: ", msg)


func _print_error(msg: String) -> void:
	printerr("ERROR: ", msg)


func _on_level_choice_id_pressed(id: int) -> void:
	_event_level = id as SentrySDK.Level
	match _event_level:
		SentrySDK.LEVEL_DEBUG:
			level_choice.text = "DEBUG"
		SentrySDK.LEVEL_INFO:
			level_choice.text = "INFO"
		SentrySDK.LEVEL_WARNING:
			level_choice.text = "WARNING"
		SentrySDK.LEVEL_ERROR:
			level_choice.text = "ERROR"
		SentrySDK.LEVEL_FATAL:
			level_choice.text = "FATAL"


func _on_capture_button_pressed() -> void:
	var event_id := SentrySDK.capture_message(message_edit.text, _event_level)
	_print_info("Captured message event. Event ID: " + event_id)


func _on_add_breadcrumb_button_pressed() -> void:
	SentrySDK.add_breadcrumb(breadcrumb_message.text, breadcrumb_category.text, SentrySDK.LEVEL_ERROR, "default")
	_print_info("Breadcrumb added.")


func _on_add_tag_button_pressed() -> void:
	SentrySDK.set_tag(tag_key.text, tag_value.text)
	if not tag_key.text.is_empty():
		_print_info("Tag added.")


func _on_crash_button_pressed() -> void:
	OS.crash("Crashing on button press")


func _on_set_context_pressed() -> void:
	if context_name.text.is_empty():
		_print_info("Please provide a name for the context.")
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
			SentrySDK.set_context(context_name.text, result)
			_print_info("Context added.")
		else:
			_print_error("Failed set context: Dictionary is expected, but found: " + type_string(typeof(result)))
	else:
		_print_error("Failed to parse expression: " + expr.get_error_text())


func _on_set_user_button_pressed() -> void:
	_print_info("Setting user info...")
	var sentry_user := SentryUser.new()
	sentry_user.id = user_id.text
	sentry_user.username = username.text
	sentry_user.email = email.text
	if infer_ip.button_pressed:
		sentry_user.infer_ip_address()
	SentrySDK.set_user(sentry_user)
	print("   ", sentry_user)
	_update_user_info()


func _on_gen_script_error_pressed() -> void:
	_print_info("Generating GDScript error...")
	# The following line should generate 2 errors:
	# script parse error and failed to load script.
	@warning_ignore("unused_variable")
	var ScriptWithErrors = load("res://script_with_errors.gd")


func _on_gen_native_error_pressed() -> void:
	_print_info("Generating native Godot error (in C++ unit)...")
	load("res://file_does_not_exist")
