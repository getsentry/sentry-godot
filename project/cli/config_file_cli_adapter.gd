class_name ConfigFileCLIAdapter
extends RefCounted
## Adapter class that reads command line arguments from a configuration file.
##
## Reads from res://launch_params.cfg and parses the "cmdline" value from the "launch" section
## into CLI-style arguments, handling quoted strings properly.


## Reads command line arguments from the launch configuration file.
## Returns CLI-style arguments as PackedStringArray, or empty array if config not found.
static func get_command_argv() -> PackedStringArray:
	var rv := PackedStringArray()

	var config := ConfigFile.new()
	var err: Error = config.load("res://launch_params.cfg")

	if err != OK:
		return rv

	var cmdline: String = config.get_value("launch", "cmdline", "")
	if cmdline.is_empty():
		return rv

	return _parse_cmdline(cmdline)


## Parses a command line string into separate arguments, handling single and double quotes.
static func _parse_cmdline(cmdline: String) -> PackedStringArray:
	var rv := PackedStringArray()
	var i := 0
	var current_arg := ""
	var in_single_quote := false
	var in_double_quote := false

	while i < cmdline.length():
		var c := cmdline[i]

		if c == '"' and not in_single_quote:
			in_double_quote = not in_double_quote
		elif c == "'" and not in_double_quote:
			in_single_quote = not in_single_quote
		elif c == ' ' and not in_single_quote and not in_double_quote:
			if not current_arg.is_empty():
				rv.append(current_arg)
				current_arg = ""
		else:
			current_arg += c

		i += 1

	# Add the last argument if it exists
	if not current_arg.is_empty():
		rv.append(current_arg)

	return rv
