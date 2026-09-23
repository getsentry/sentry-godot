extends VBoxContainer

const SECTION_MINIMUM_WIDTH := 250.0
const EMPOWER_PLANT_EXISTING_PAGE_URL := "https://flask.empower-plant.com/products"
const EMPOWER_PLANT_MISSING_PAGE_URL := "https://flask.empower-plant.com/not-found"

@onready var sections: HFlowContainer = %Sections
@onready var message_edit: LineEdit = %MessageEdit
@onready var level_choice: MenuButton = %LevelChoice
@onready var http_request: SentryHTTPRequest = %SentryHTTPRequest
@onready var request_existing_page_button: Button = %RequestExistingPageButton
@onready var request_missing_page_button: Button = %RequestMissingPageButton

var _event_level: SentrySDK.Level
var _user_feedback_gui: Control
var _sending_metrics := false
var _requested_url: String


func _ready() -> void:
	_init_user_feedback_gui()
	_init_level_choice_popup()


func add_dotnet_actions() -> void:
	var panel := PanelContainer.new()
	panel.custom_minimum_size.x = SECTION_MINIMUM_WIDTH
	panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	panel.theme_type_variation = &"SectionPanel"
	var dotnet_scene: PackedScene = load("res://views/dotnet_actions.tscn")
	panel.add_child(dotnet_scene.instantiate())
	sections.add_child(panel)
	sections.move_child(panel, 0)


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


func _on_send_metrics_button_pressed() -> void:
	_sending_metrics = !_sending_metrics
	if _sending_metrics:
		%SendMetricsButton.text = "Stop metrics"
		DemoOutput.print_info("Started sending metrics.")
		%MetricsTimer.start(1.0)
		_on_metrics_timer_timeout()
	else:
		%SendMetricsButton.text = "Start metrics"
		DemoOutput.print_info("Stopped sending metrics.")
		%MetricsTimer.stop()


func _on_metrics_timer_timeout() -> void:
	SentrySDK.metrics.gauge("static_memory_usage", OS.get_static_memory_usage(), "byte")


func _on_crash_with_null_dereference_button_pressed() -> void:
	DemoOutput.print_info("Crashing app with null dereference...")
	SentrySDK.bad_code.crash_with_null_dereference()


func _on_crash_with_stack_overflow_button_pressed() -> void:
	DemoOutput.print_info("Crashing app with stack overflow...")
	SentrySDK.bad_code.crash_with_stack_overflow()


func _on_crash_with_abort_button_pressed() -> void:
	DemoOutput.print_info("Crashing app with abort...")
	SentrySDK.bad_code.crash_with_abort()


func _on_crash_with_div_by_zero_button_pressed() -> void:
	DemoOutput.print_info("Crashing app with division by zero...")
	SentrySDK.bad_code.crash_with_division_by_zero()


func _on_request_existing_page_button_pressed() -> void:
	_request_web_page(EMPOWER_PLANT_EXISTING_PAGE_URL)


func _on_request_missing_page_button_pressed() -> void:
	_request_web_page(EMPOWER_PLANT_MISSING_PAGE_URL)


func _request_web_page(url: String) -> void:
	_requested_url = url
	request_existing_page_button.disabled = true
	request_missing_page_button.disabled = true
	DemoOutput.print_info("Requesting %s..." % url)

	var start_error: Error = http_request.request(url)
	if start_error != OK and start_error != ERR_CANT_CONNECT:
		request_existing_page_button.disabled = false
		request_missing_page_button.disabled = false
		DemoOutput.print_err(
			"Failed to start the request to %s: %s"
			% [url, error_string(start_error)]
		)


func _on_http_request_completed(
		result: int,
		response_code: int,
		_headers: PackedStringArray,
		body: PackedByteArray,
) -> void:
	request_existing_page_button.disabled = false
	request_missing_page_button.disabled = false
	if result != HTTPRequest.RESULT_SUCCESS:
		DemoOutput.print_err(
			"The request to %s failed with result %d."
			% [_requested_url, result]
		)
		return

	DemoOutput.print_info(
		"The request to %s responded with HTTP %d (%d bytes)."
		% [_requested_url, response_code, body.size()]
	)
