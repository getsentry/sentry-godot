class_name AndroidCLIAdapter
extends RefCounted
## Adapter class that converts Android intent extras into CLI-style arguments.
##
## Expects Android intent extras as key-value pairs: [br]
##   command: name of the command to run. [br]
## 	 arg0: first argument to the command. [br]
##   arg1: second argument, etc... [br]



# Reads Android intent extras and returns CLI-style arguments as PackedStringArray.
# Supports --es command and --es arg0, arg1, arg2...
static func get_command_argv() -> PackedStringArray:
	var rv := PackedStringArray()
	var extras: Dictionary = _get_android_intent_extras()

	for i in range(10):
		var key := "arg%d" % i
		if extras.has(key):
			rv.append(extras[key])
	return rv


# Returns intent extras from AndroidRuntime (strings-only).
static func _get_android_intent_extras() -> Dictionary:
	if not Engine.has_singleton("AndroidRuntime"):
		return {}

	var android_runtime = Engine.get_singleton("AndroidRuntime")
	var activity = android_runtime.getActivity()
	if not activity:
		return {}
	var intent = activity.getIntent()
	if not intent:
		return {}
	var extras = intent.getExtras()
	if not extras:
		return {}

	# Convert Java map to Godot Dictionary
	var keys = extras.keySet().toArray()
	var rv: Dictionary = {}
	for i in range(keys.size()):
		var key: String = keys[i].toString()
		var raw_value = extras.get(key)
		var value: String = raw_value.toString() if raw_value != null else ""
		rv[key] = value

	return rv
