extends Node

func _ready() -> void:
	if OS.get_name() in ["Android", "iOS"]:
		get_tree().change_scene_to_file.call_deferred("res://mobile.tscn")
	else:
		get_tree().change_scene_to_file.call_deferred("res://desktop.tscn")
