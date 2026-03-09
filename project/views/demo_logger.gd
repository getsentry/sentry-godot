class_name DemoLogger
extends Logger

signal message_logged(text: String)
signal pool_trimmed()

const MAX_MESSAGES: int = 2000
const KEEP_MESSAGES: int = 1000

var message_pool: PackedStringArray

var _ansi_escape_regex: RegEx = RegEx.create_from_string(r"\x1b\[[0-9;]*[a-zA-Z]")

func _log_message(message: String, _error: bool) -> void:
	message = _ansi_escape_regex.sub(message, "", true)
	message_pool.append(message)
	if message_pool.size() >= MAX_MESSAGES:
		message_pool = message_pool.slice(KEEP_MESSAGES)
		pool_trimmed.emit.call_deferred()
	else:
		message_logged.emit.call_deferred(message)
