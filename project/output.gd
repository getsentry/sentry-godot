extends RichTextLabel

var _log_file: FileAccess


func _init() -> void:
	var fn: String = ProjectSettings.get("debug/file_logging/log_path")
	_log_file = FileAccess.open(fn, FileAccess.READ)

	if not _log_file:
		push_error("Log file not found.")
		return

	var tween := create_tween().set_loops()
	tween.tween_interval(0.1)
	tween.tween_callback(_read_log)


func _read_log() -> void:
	while _log_file.get_position() < _log_file.get_length():
		var line: String = _log_file.get_line() + "\n"
		text += line
