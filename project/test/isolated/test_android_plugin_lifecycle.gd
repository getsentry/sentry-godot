extends GdUnitTestSuite
## We attach a script to the JNI plugin to shut down Sentry before the plugin is destroyed.
## Godot destroys JNI singletons before scene-tree cleanup, so waiting for that cleanup is too late.
## This test sends the notification without freeing the plugin and verifies SDK, scope, and callback cleanup.


func before(_do_skip = not OS.has_feature("android"),
		_skip_reason = "Requires the Android JNI plugin.") -> void:
	pass


func test_plugin_predelete_runs_full_sdk_shutdown() -> void:
	var plugin: Object = Engine.get_singleton("SentryAndroidGodotPlugin")
	var configured_options: Array[SentryOptions] = []
	SentrySDK.init(func(value: SentryOptions) -> void:
		value.shutdown_timeout_ms = 0
		value.before_send = func(_event: SentryEvent) -> SentryEvent:
			return null
		configured_options.append(value)
	)
	var options: SentryOptions = configured_options[0]
	var scope_ref: WeakRef = weakref(SentrySDK.get_current_scope())
	assert_bool(SentrySDK.is_enabled()).is_true()
	assert_object(plugin.get_script()).is_not_null()
	assert_bool(options.before_send.is_valid()).is_true()

	plugin.notification(NOTIFICATION_PREDELETE)
	assert_bool(SentrySDK.is_enabled()).is_false()
	assert_object(scope_ref.get_ref()).is_null()
	assert_bool(options.before_send.is_valid()).is_false()
