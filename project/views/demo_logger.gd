class_name DemoLogger
extends Logger

signal message_logged(text: String)
signal pool_trimmed()

const MAX_MESSAGES: int = 2000
const KEEP_MESSAGES: int = 1000

var message_pool: PackedStringArray

var _ansi_escape_regex: RegEx = RegEx.create_from_string(r"\x1b\[[0-9;]*[a-zA-Z]")


func _log_message(message: String, _error: bool) -> void:
	_add_message.call_deferred(message)


func _add_message(message: String) -> void:
	if message_pool.size() >= MAX_MESSAGES:
		message_pool = message_pool.slice(KEEP_MESSAGES)
		pool_trimmed.emit()
	message = _ansi_escape_regex.sub(message, "", true)
	message_pool.append(message)
	message_logged.emit(message)
