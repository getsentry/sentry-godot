extends VBoxContainer

@onready var message_edit: LineEdit = %MessageEdit
@onready var level_choice: MenuButton = %LevelChoice
@onready var user_id: LineEdit = %UserID
@onready var username: LineEdit = %Username
@onready var email: LineEdit = %Email
@onready var infer_ip: CheckBox = %InferIP

var _event_level: SentrySDK.Level


func _ready() -> void:
	level_choice.get_popup().id_pressed.connect(_on_level_choice_id_pressed)
	_init_level_choice_popup()
	_update_user_info()


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
	DemoOutput.print_info("Captured message event. Event ID: " + event_id)


func _on_crash_button_pressed() -> void:
	OS.crash("Crashing on button press")


func _on_set_user_button_pressed() -> void:
	DemoOutput.print_info("Setting user info...")
	var sentry_user := SentryUser.new()
	sentry_user.id = user_id.text
	sentry_user.username = username.text
	sentry_user.email = email.text
	if infer_ip.button_pressed:
		sentry_user.infer_ip_address()
	SentrySDK.set_user(sentry_user)
	DemoOutput.print_extra(str(sentry_user))
	_update_user_info()


func _on_gen_script_error_pressed() -> void:
	DemoOutput.print_info("Generating GDScript error...")
	# The following line should generate 2 errors:
	# script parse error and failed to load script.
	@warning_ignore("unused_variable")
	var ScriptWithErrors = load("res://script_with_errors.gd")


func _on_gen_native_error_pressed() -> void:
	DemoOutput.print_info("Generating native Godot error (in C++ unit)...")
	load("res://file_does_not_exist")
