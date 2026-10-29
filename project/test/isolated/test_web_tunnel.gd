extends GdUnitTestSuite


@warning_ignore("unused_parameter")
func before(_do_skip = not OS.has_feature("web"),
		_skip_reason = "Managed tunneling uses the Web JavaScript transport.") -> void:
	pass


func before_test() -> void:
	JavaScriptBridge.eval("window.__tunnelUploadUrl = ''")


func after_test() -> void:
	SentrySDK.close()
	while SentrySDK.is_enabled():
		await get_tree().process_frame


func _capture_with_tunnel(enabled: bool, path: String, base_path: String, trailing_slash: bool) -> String:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.dsn = "https://public@example.invalid/42"
		options.debug = false
		# Keep envelope light.
		options.attach_log = false
		options.attach_scene_tree = false
		options.attach_screenshot = false
		options.godot_logger.enabled = false
		options.web.enable_tunnel = enabled
		options.web.tunnel_path = path
		options.web.tunnel_base_path = base_path
		options.web.tunnel_trailing_slash = trailing_slash
	)
	SentrySDK.capture_message("web tunnel regression")
	var deadline: int = Time.get_ticks_msec() + 5000
	while JavaScriptBridge.eval("window.__tunnelUploadUrl === ''") and Time.get_ticks_msec() < deadline:
		await get_tree().process_frame
	return JavaScriptBridge.eval("window.__tunnelUploadUrl")


func test_disabled_tunnel_ignores_managed_options() -> void:
	var url: String = await _capture_with_tunnel(false, "/events", "/game", true)
	assert_str(url).starts_with("https://example.invalid/api/42/envelope/")


func test_empty_path_uses_the_dsn() -> void:
	var url: String = await _capture_with_tunnel(true, "", "", false)
	assert_str(url).is_equal("/1qk9y43")


func test_derived_path_with_base_path_and_trailing_slash() -> void:
	var url: String = await _capture_with_tunnel(true, "", "/game", true)
	assert_str(url).is_equal("/game/1qk9y43/")


func test_explicit_path() -> void:
	var url: String = await _capture_with_tunnel(true, "/events", "", false)
	assert_str(url).is_equal("/events")


func test_explicit_path_with_base_path_and_trailing_slash() -> void:
	var url: String = await _capture_with_tunnel(true, "/events", "/game", true)
	assert_str(url).is_equal("/game/events/")


func test_path_containing_a_trailing_slash() -> void:
	var url: String = await _capture_with_tunnel(true, "/events/", "", false)
	assert_str(url).is_equal("/events/")
