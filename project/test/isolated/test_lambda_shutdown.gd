extends GdUnitTestSuite
## Regression test for #797: a GDScript lambda set as a Sentry callback could crash on exit.
## This suite is isolated, so the runner catches such a crash via the exit code.


## Assigns a lambda to each Callable on Sentry options object and its nested option objects.
func _assign_lambdas_recursive(object: Object, depth: int = 0) -> void:
	const MAX_DEPTH := 4
	if object == null or depth > MAX_DEPTH:
		return

	var properties := object.get_property_list()

	for property in properties:
		if int(property["type"]) == TYPE_CALLABLE:
			var prop_name: StringName = property["name"]
			object.set(prop_name, func(_arg1 = null, _arg2 = null, _arg3 = null): pass)

	for property in properties:
		if int(property["type"]) == TYPE_OBJECT:
			# Assign callables in nested objects such as `experimental` options.
			_assign_lambdas_recursive(object.get(property["name"]), depth + 1)


func test_lambda_callback_does_not_crash_on_exit() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.godot_logger.enabled = false  # mute logging
		_assign_lambdas_recursive(options)
	)
	assert_bool(SentrySDK.is_enabled()).is_true()
	# Leave the lambdas assigned so they're still set at shutdown, where the crash happened.
