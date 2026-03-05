extends Node

@onready var emit_timer: Timer = %EmitTimer

func _ready() -> void:
	DemoOutput.print_info("Preparing to send metrics.")
	emit_timer.start(1.0) # each second

func _on_emit_timer_timeout() -> void:
	SentrySDK.metrics.gauge("static_memory_usage", OS.get_static_memory_usage(), "byte")
