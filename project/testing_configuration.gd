extends RefCounted


## Detects whether an isolated test suite is running and calls its static `configure_options()` method.
## - Since SentryOptions can be configured only once, such test suites must be executed separately.
## - This method is called from "example_configuration.gd".
static func configure_options(options: SentryOptions):
	var args: PackedStringArray = OS.get_cmdline_args()
	var path: String
	for arg: String in args:
		if arg.begins_with("-gtest="):
			path = arg.trim_prefix("-gtest=").lstrip('"').rstrip('"')
			break
	if not path.ends_with(".gd"):
		return
	if not FileAccess.file_exists(path):
		printerr("Test file not found: " + path)
		return
	var scr: GDScript = load(path)
	if scr.has_method(&"configure_options"):
		scr.call(&"configure_options", options)
