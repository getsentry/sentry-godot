extends RichTextLabel

var _log_file: FileAccess
var _regex: RegEx
var _tween: Tween

@onready var msg_copied: PanelContainer = %MsgCopied


func _init() -> void:
	var fn: String = ProjectSettings.get("debug/file_logging/log_path")
	_log_file = FileAccess.open(fn, FileAccess.READ)

	if not _log_file:
		push_error("Log file not found.")
		return

	var tween := create_tween().set_loops()
	tween.tween_interval(0.1)
	tween.tween_callback(_read_log)

	_regex = RegEx.create_from_string(r"\b([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-4[0-9a-fA-F]{3}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})\b")


func _ready() -> void:
	msg_copied.modulate = Color.TRANSPARENT

	prints(_regex.search(r"[example_configuration.gd] Processing event: c32cf0d5-4da3-4d9a-c502-c5f4007fc686"))


func _read_log() -> void:
	while _log_file.get_position() < _log_file.get_length():
		var line: String = _log_file.get_line() + "\n"
		text += _linkify_uuids(line)


func _linkify_uuids(line: String) -> String:
	line = _regex.sub(line, r"[url]$1[/url]", true)
	return line


func _pop_copied_message():
	if _tween and _tween.is_valid():
		_tween.kill()
	_tween = create_tween()
	_tween.tween_property(msg_copied, "modulate", Color.WHITE, 0.1)
	_tween.tween_interval(3.0)
	_tween.tween_property(msg_copied, "modulate", Color.TRANSPARENT, 0.1)


func _on_meta_clicked(meta: Variant) -> void:
	DisplayServer.clipboard_set(str(meta))
	_pop_copied_message()
