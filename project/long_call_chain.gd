extends Node

static func road_to_error_1() -> void:
	road_to_error_2()

static func road_to_error_2() -> void:
	road_to_error_3()

static func road_to_error_3() -> void:
	road_to_error_4()

static func road_to_error_4() -> void:
	road_to_error_5()

static func road_to_error_5() -> void:
	@warning_ignore("unused_variable")
	var ScriptWithErrors = load("res://script_with_errors.gd")
	
