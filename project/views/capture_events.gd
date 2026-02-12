extends VBoxContainer

@onready var message_edit: LineEdit = %MessageEdit
@onready var level_choice: MenuButton = %LevelChoice
@onready var user_id: LineEdit = %UserID
@onready var username: LineEdit = %Username
@onready var email: LineEdit = %Email
@onready var infer_ip: CheckBox = %InferIP

var _event_level: SentrySDK.Level

var _user_feedback_gui: Control


func _ready() -> void:
	_init_user_feedback_gui()
	_init_level_choice_popup()
	_init_user_info()


## Initialize User Feedback UI
func _init_user_feedback_gui() -> void:
	_user_feedback_gui = load("res://addons/sentry/user_feedback/user_feedback_gui.tscn").instantiate()
	_user_feedback_gui.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	get_owner().add_child.call_deferred(_user_feedback_gui)
	_user_feedback_gui.hide()


func _init_level_choice_popup() -> void:
	var popup: PopupMenu = level_choice.get_popup()
	popup.add_item("DEBUG", SentrySDK.LEVEL_DEBUG)
	popup.add_item("INFO", SentrySDK.LEVEL_INFO)
	popup.add_item("WARNING", SentrySDK.LEVEL_WARNING)
	popup.add_item("ERROR", SentrySDK.LEVEL_ERROR)
	popup.add_item("FATAL", SentrySDK.LEVEL_FATAL)

	popup.id_pressed.connect(_on_level_choice_id_pressed)
	_on_level_choice_id_pressed(SentrySDK.LEVEL_INFO)


func _init_user_info() -> void:
	var user := SentryUser.create_default()
	SentrySDK.set_user(user)

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


func _on_set_user_button_pressed() -> void:
	DemoOutput.print_info("Setting user info...")
	var user := SentryUser.new()
	user.id = user_id.text
	user.username = username.text
	user.email = email.text
	if infer_ip.button_pressed:
		user.infer_ip_address()
	SentrySDK.set_user(user)
	DemoOutput.print_extra(str(user))


func _on_gen_script_error_pressed() -> void:
	var thread_name: String = "main"
	_generate_script_error(thread_name)


func _on_gen_script_error_from_thread_pressed() -> void:
	var thread := Thread.new()
	thread.start(_generate_script_error.bind("worker"))
	thread.wait_to_finish()


func _on_gen_native_error_pressed() -> void:
	_generate_native_error()


func _generate_script_error(thread_name: String) -> void:
	DemoOutput.print_info("Generating GDScript error from %s thread..." % thread_name)
	# The following line should generate 2 errors:
	# script parse error and failed to load script.
	@warning_ignore("unused_variable")
	var ScriptWithErrors = load("res://script_with_errors.gd")


func _generate_native_error() -> void:
	DemoOutput.print_info("Generating native Godot error (in C++ unit)...")
	load("res://file_does_not_exist")


func _on_user_feedback_button_pressed() -> void:
	_user_feedback_gui.show()


func _on_crash_with_null_dereference_button_pressed() -> void:
	DemoOutput.print_info("Crashing app with null dereference...")
	Kaboom.crash_with_null_dereference()


func _on_crash_with_stack_overflow_button_pressed() -> void:
	DemoOutput.print_info("Crashing app with stack overflow...")
	Kaboom.crash_with_stack_overflow()


func _on_crash_with_abort_button_pressed() -> void:
	DemoOutput.print_info("Crashing app with abort...")
	Kaboom.crash_with_abort()


func _on_crash_with_div_by_zero_button_pressed() -> void:
	DemoOutput.print_info("Crashing app with division by zero...")
	Kaboom.crash_with_division_by_zero()
