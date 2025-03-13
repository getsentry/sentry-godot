extends RefCounted


## Detects whether an isolated test suite is running and calls its static `configure_options()` method.
## - Since SentryOptions can be configured only once, such test suites must be executed separately.
## - This method is called from "example_configuration.gd".
static func configure_options(options: SentryOptions):
	var args: PackedStringArray = OS.get_cmdline_args()
	var idx := args.find("-a")
	if idx == -1 or args.size() == idx + 1:
		return
	var path := args[idx + 1]
	if not path.ends_with(".gd"):
		return
	if not FileAccess.file_exists(path):
		return
	var scr: GDScript = load(path)
	if scr.has_method(&"configure_options"):
		scr.call(&"configure_options", options)
