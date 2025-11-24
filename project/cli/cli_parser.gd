class_name CLIParser
extends RefCounted
## CLI Parser utility for command-line argument parsing and execution.
##
## Uses "./binary -- COMMAND [ARGS...]" format with automatic argument parsing
## from function metadata.


enum ExitCode {
	SUCCESS = 0,
	FAILURE = 1,
	INVALID_ARGUMENTS = 2
}

var exit_code: int = 0

var _commands: Dictionary = {}


## Determines whether the CLI parser should execute based on command-line arguments.
static func should_execute() -> bool:
	return not OS.get_cmdline_user_args().is_empty() \
		or not AndroidCLIAdapter.get_command_argv().is_empty()


## Registers a new CLI command with the parser.
func add_command(p_name: String, p_callable: Callable, p_description: String = "") -> void:
	_commands[p_name] = {
		callable = p_callable,
		description = p_description
	}


## Parses command-line arguments and executes the corresponding command if found.
##
## Returns true if a command was found and executed, false if no arguments were provided.
## Sets the exit_code property based on execution results.
func check_and_execute_cli() -> bool:
	var args: PackedStringArray = OS.get_cmdline_user_args()

	if OS.get_name() == "Android":
		args.append_array(AndroidCLIAdapter.get_command_argv())

	if args.is_empty():
		return false

	var command_name: String = args[0]
	var command_args: PackedStringArray = args.slice(1)

	exit_code = await _run_command(command_name, command_args)
	return true


## Generates help text for all registered commands.
func generate_help() -> String:
	var help_text := "Available commands:\n\n"

	var command_names: Array = _commands.keys()
	command_names.sort()

	for command_name in command_names:
		var command_data: Dictionary = _commands[command_name]
		var callable: Callable = command_data.callable
		var description: String = command_data.get("description", "No description")

		help_text += "  %s\n" % command_name
		help_text += "    %s\n" % description

		# Show arguments if any
		var method_info := _get_method_info(callable)
		if not method_info.is_empty():
			var method_args: Array = method_info.get("args", [])
			var num_with_defaults: int = method_info.get("default_args", []).size()
			var required_args: int = method_args.size() - num_with_defaults
			var arg_strings: PackedStringArray = []

			for i in range(method_args.size()):
				var arg_info: Dictionary = method_args[i]
				var type_name: String = _get_type_name(arg_info.get("type", TYPE_NIL))
				var arg_name: String = arg_info.get("name", "unknown")
				var optional := " (optional)" if i >= required_args else ""
				arg_strings.append("%s: %s%s" % [arg_name, type_name, optional])

			if not arg_strings.is_empty():
				help_text += "    Arguments: %s\n" % ", ".join(arg_strings)

		help_text += "\n"

	help_text += "Usage: godot --headless --path <project_path> -- COMMAND [ARGS...]\n"
	help_text += "Example: godot --headless --path ./project -- message-capture \"Hello, World\""

	return help_text


func _run_command(p_command_name: String, p_args: PackedStringArray) -> int:
	if not _commands.has(p_command_name):
		printerr("Unknown command: " + p_command_name)
		printerr("Use 'help' to see available commands")
		return ExitCode.FAILURE

	var command_data: Dictionary = _commands[p_command_name]
	var callable: Callable = command_data.callable

	if not callable.is_valid():
		printerr("Error: Invalid callable for command: " + p_command_name)
		return ExitCode.FAILURE

	var method_info: Dictionary = _get_method_info(callable)
	if method_info.is_empty():
		printerr("Error: Could not get method info for command: " + p_command_name)
		return ExitCode.FAILURE

	# Parse arguments based on function arguments
	var parse_result := _parse_arguments(method_info, p_args, p_command_name)
	if not parse_result.success:
		# Error already printed in _parse_arguments
		return ExitCode.INVALID_ARGUMENTS

	# Call the function with parsed arguments
	var status: int = await callable.callv(parse_result.args)
	return status


func _get_method_info(p_callable: Callable) -> Dictionary:
	var object: Object = p_callable.get_object()
	var method_name: StringName = p_callable.get_method()

	if object == null or method_name.is_empty():
		return {}

	# Try to get method list, handle potential errors
	var method_list: Array[Dictionary]
	if object.has_method("get_method_list"):
		method_list = object.get_method_list()
	else:
		return {}

	for method_info: Dictionary in method_list:
		if method_info.get("name", "") == method_name:
			return method_info

	return {}


func _parse_arguments(p_method_info: Dictionary, p_args: PackedStringArray, p_command_name: String) -> Dictionary:
	var method_args: Array = p_method_info.get("args", [])
	var parsed_args: Array = []

	# Calculate required and optional arguments using method info
	var max_args: int = method_args.size()
	var num_with_defaults: int = p_method_info.get("default_args", []).size()
	var required_args: int = max_args - num_with_defaults

	if p_args.size() < required_args or p_args.size() > max_args:
		var msg := "Error: Command '%s' takes %d arguments (%d required), but got %d" % [p_command_name, max_args, required_args, p_args.size()]
		if not method_args.is_empty():
			msg += "\nExpected arguments:"
			for i in range(method_args.size()):
				var arg_info: Dictionary = method_args[i]
				var type_name: String = _get_type_name(arg_info.get("type", TYPE_NIL))
				var arg_name: String = arg_info.get("name", "unknown")
				var optional := " (optional)" if i >= required_args else ""
				msg += "\n  %s (%s)%s" % [arg_name, type_name, optional]
		printerr(msg)
		return {success = false, args = []}

	# Convert provided string arguments to appropriate types
	for i in range(p_args.size()):
		var arg_info: Dictionary = method_args[i]
		var arg_value: String = p_args[i]
		var converted_value: Variant = _convert_argument(arg_value, arg_info.get("type", TYPE_NIL), arg_info.get("name", "unknown"))

		if converted_value == null:
			printerr("Error: Could not convert argument '%s' to required type for argument '%s'" % [arg_value, arg_info.get("name", "unknown")])
			return {success = false, args = []}

		parsed_args.append(converted_value)

	return {success = true, args = parsed_args}


func _convert_argument(p_value: String, p_type: int, p_arg_name: String) -> Variant:
	# Handle empty values
	if p_value.is_empty() and p_type != TYPE_STRING:
		printerr("Error: Empty value provided for argument '%s'" % p_arg_name)
		return null

	match p_type:
		TYPE_STRING:
			return p_value
		TYPE_INT:
			if p_value.is_valid_int():
				return p_value.to_int()
			else:
				printerr("Error: '%s' is not a valid integer for argument '%s'" % [p_value, p_arg_name])
				return null
		TYPE_FLOAT:
			if p_value.is_valid_float():
				return p_value.to_float()
			else:
				printerr("Error: '%s' is not a valid float for argument '%s'" % [p_value, p_arg_name])
				return null
		TYPE_BOOL:
			var lower_value := p_value.to_lower().strip_edges()
			if lower_value in ["true", "1", "yes", "on"]:
				return true
			elif lower_value in ["false", "0", "no", "off"]:
				return false
			else:
				printerr("Error: '%s' is not a valid boolean for argument '%s'. Use: true/false, 1/0, yes/no, on/off" % [p_value, p_arg_name])
				return null
		TYPE_NIL:
			printerr("Warning: Argument '%s' has unknown type, treating as string" % p_arg_name)
			return p_value
		_:
			# For other types, try to convert as string with warning
			printerr("Warning: Unsupported argument type %d for '%s', treating as string" % [p_type, p_arg_name])
			return p_value


func _get_type_name(p_type: int) -> String:
	match p_type:
		TYPE_STRING: return "String"
		TYPE_INT: return "int"
		TYPE_FLOAT: return "float"
		TYPE_BOOL: return "bool"
		TYPE_NIL: return "unknown"
		_: return "Variant"
