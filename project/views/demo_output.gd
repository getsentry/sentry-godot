class_name DemoOutput
extends RichTextLabel

enum Level {
	DEBUG,
	INFO,
	WARNING,
	ERROR,
	FATAL
}

var minimum_level: Level = Level.INFO

var _uuid_regex: RegEx = RegEx.create_from_string(r"\b([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-4[0-9a-fA-F]{3}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})\b")
var _tween: Tween
var _last_level

@onready var msg_copied: PanelContainer = %MsgCopied
@onready var verbosity_menu: MenuButton = %VerbosityMenu


static func print_info(msg: String) -> void:
	print("INFO: " + msg)


static func print_err(msg: String) -> void:
	printerr("ERROR: " + msg)


static func print_line(msg: String) -> void:
	print(msg)


static func print_extra(msg: String) -> void:
	print("   ", msg)


func _ready() -> void:
	get_tree().logger.message_logged.connect(_push_message)
	get_tree().logger.pool_trimmed.connect(_pull_log_messages)
	_pull_log_messages()

	msg_copied.modulate = Color.TRANSPARENT

	var popup := verbosity_menu.get_popup()
	for l in Level:
		popup.add_item(l)
	verbosity_menu.text = Level.keys()[minimum_level]
	popup.id_pressed.connect(
		func(id):
			minimum_level = id
			verbosity_menu.text = Level.keys()[minimum_level]
			_pull_log_messages()
	)


func _pull_log_messages() -> void:
	text = ""
	_last_level = null
	for line in get_tree().logger.message_pool:
		_push_message(line)


func _push_message(message: String) -> void:
	var level: Level = _get_message_level(message)
	if level >= minimum_level:
		text += _linkify_uuids(message)


func _get_message_level(line: String) -> Level:
	for level_string in Level.keys():
		if line.trim_prefix("Sentry: ").begins_with(level_string):
			_last_level = Level[level_string]
			return Level[level_string]
		elif line.begins_with(" ") and _last_level != null:
			return _last_level
	_last_level = null
	return Level.INFO


func _linkify_uuids(line: String) -> String:
	line = _uuid_regex.sub(line, r"[url]$1[/url]", true)
	return line


func _pop_copied_toast():
	if _tween and _tween.is_valid():
		_tween.kill()
	_tween = create_tween()
	_tween.tween_property(msg_copied, "modulate", Color.WHITE, 0.1)
	_tween.tween_interval(3.0)
	_tween.tween_property(msg_copied, "modulate", Color.TRANSPARENT, 0.1)


func _on_meta_clicked(meta: Variant) -> void:
	DisplayServer.clipboard_set(str(meta))
	_pop_copied_toast()
