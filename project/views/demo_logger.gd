class_name DemoLogger
extends Logger

signal message_logged(text: String)

var message_pool: PackedStringArray

var _ansi_escape_regex: RegEx = RegEx.create_from_string(r"\x1b\[[0-9;]*[a-zA-Z]")

func _log_message(message: String, _error: bool) -> void:
	message = _ansi_escape_regex.sub(message, "", true)
	message_pool.append(message)
	message_logged.emit.call_deferred(message)
