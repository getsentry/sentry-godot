class_name DemoLogger
extends Logger

signal message_logged(text: String)

var message_pool: PackedStringArray

func _log_message(message: String, _error: bool) -> void:
	message_pool.append(message)
	message_logged.emit.call_deferred(message)
