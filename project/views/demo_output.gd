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

var _log_file: FileAccess
var _uuid_regex: RegEx
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


func _init() -> void:
	_open_log_file()

	var tween := create_tween().set_loops()
	tween.tween_interval(0.1)
	tween.tween_callback(_read_log)

	_uuid_regex = RegEx.create_from_string(r"\b([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-4[0-9a-fA-F]{3}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})\b")


func _ready() -> void:
	msg_copied.modulate = Color.TRANSPARENT

	var popup := verbosity_menu.get_popup()
	for l in Level:
		popup.add_item(l)
	verbosity_menu.text = Level.keys()[minimum_level]
	popup.id_pressed.connect(
		func(id):
			minimum_level = id
			verbosity_menu.text = Level.keys()[minimum_level]
			_open_log_file()
	)


func _open_log_file() -> void:
	text = ""
	var fn: String = ProjectSettings.get("debug/file_logging/log_path")
	_log_file = FileAccess.open(fn, FileAccess.READ)
	if not _log_file:
		push_error("Log file not found.")
		return


func _get_level(line: String):
	for level_string in Level.keys():
		if line.trim_prefix("Sentry: ").begins_with(level_string):
			_last_level = Level[level_string]
			return Level[level_string]
		elif line.begins_with(" "):
			return _last_level
	_last_level = null
	return null


func _read_log() -> void:
	while _log_file.get_position() < _log_file.get_length():
		var line: String = _log_file.get_line() + "\n"
		var level = _get_level(line)
		if level != null and level < minimum_level:
			continue
		text += _linkify_uuids(line)


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
