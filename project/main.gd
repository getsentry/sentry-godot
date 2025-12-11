extends Node

@onready var cli_commands: CLICommands = %CLICommands


func _ready() -> void:
	SentrySDK.logger.info("Starting UI on %s", [OS.get_name()])

	if await cli_commands.check_and_execute_cli():
		# Quit if a CLI command was executed
		if OS.get_name() == "iOS":
			OS.kill(OS.get_process_id())
		else:
			get_tree().quit(cli_commands.exit_code)
	elif OS.get_name() in ["Android", "iOS"]:
		# Continue with mobile UI
		get_tree().change_scene_to_file.call_deferred("res://mobile.tscn")
	else:
		# Continue with desktop UI
		get_tree().change_scene_to_file.call_deferred("res://desktop.tscn")
