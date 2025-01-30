class_name TestingConfiguration
extends RefCounted


## When running a single test suite (a single GDScript file), invoke its static
## `configure_options()` method if it exists.
## Options can only be configured once, so such test suites must be executed separately.
static func configure_options(options: SentryOptions):
	var args: PackedStringArray = OS.get_cmdline_args()
	var idx := args.find("-a")
	if idx == -1 or args.size() == idx + 1:
		return
	var path := "res://" + args[idx + 1]
	if not path.ends_with(".gd"):
		return
	if not FileAccess.file_exists(path):
		return
	var scr: GDScript = load(path)
	if scr.has_method(&"configure_options"):
		scr.call(&"configure_options", options)
