extends GdUnitTestSuite

const NEWLINE_BYTE: int = 10

var _capture: HTTPRequestCapture
var _items: Array[Dictionary] = []


func before_test() -> void:
	_items.clear()

	_capture = HTTPRequestCapture.new()
	add_child(_capture)
	assert_int(_capture.start()).is_equal(OK)

	SentrySDK.init(func(options: SentryOptions) -> void:
		options.dsn = _capture.get_dsn()
		options.release = "session-test"
		options.auto_session_tracking = false
		options.debug = false
		options.attach_log = false
		options.attach_scene_tree = false
		options.attach_screenshot = false
		options.godot_logger.log_mask = 0
		options.godot_logger.breadcrumb_mask = 0
		options.traces_sample_rate = 0.0
		options.shutdown_timeout_ms = 0
	)
	assert_bool(SentrySDK.is_enabled()).is_true()


func after_test() -> void:
	SentrySDK.close()
	var deadline: int = Time.get_ticks_msec() + 5000
	while SentrySDK.is_enabled() and Time.get_ticks_msec() < deadline:
		# On Web, close() is async, so we need to wait.
		await get_tree().process_frame
	assert_bool(SentrySDK.is_enabled()).is_false()
	_capture.stop()
	_capture.free()


func assert_json(value: Variant) -> JSONAssert:
	return JSONAssert.new(value)


func _read_items(item_type: String, predicate: Callable = Callable()) -> Array[Dictionary]:
	for request: Dictionary in _capture.take_requests():
		var body: PackedByteArray = request.body
		if request.headers.get("content-encoding", "") == "gzip":
			body = body.decompress_dynamic(4 * 1024 * 1024, FileAccess.COMPRESSION_GZIP)
		# An envelope starts with a JSON header followed by a newline.
		var offset: int = body.find(NEWLINE_BYTE) + 1
		while offset > 0 and offset < body.size():
			# Each item starts with a JSON header followed by a newline, then its payload.
			var header_end: int = body.find(NEWLINE_BYTE, offset)
			if header_end < 0:
				break
			var header: Dictionary = JSON.parse_string(body.slice(offset, header_end).get_string_from_utf8())
			offset = header_end + 1
			var payload_end: int = offset + int(header.length) if header.has("length") else body.find(NEWLINE_BYTE, offset)
			if payload_end < 0:
				payload_end = body.size()
			if header.type in ["session", "event"]:
				var payload: Dictionary = JSON.parse_string(body.slice(offset, payload_end).get_string_from_utf8())
				_items.append({"type": header.type, "payload": payload})
			offset = payload_end + 1
	var payloads: Array[Dictionary] = []
	for item: Dictionary in _items:
		if item.type == item_type and (not predicate.is_valid() or predicate.call(item.payload)):
			payloads.append(item.payload)
	return payloads


func _wait_for_items(item_type: String, count: int, predicate: Callable = Callable()) -> Array[Dictionary]:
	var deadline: int = Time.get_ticks_msec() + 5000
	var items: Array[Dictionary] = _read_items(item_type, predicate)
	while items.size() < count and Time.get_ticks_msec() < deadline:
		await get_tree().process_frame
		items = _read_items(item_type, predicate)
	assert_int(items.size()).is_greater_equal(count)
	return items


func test_disabled_auto_tracking_sends_no_sessions() -> void:
	SentrySDK.capture_message("Automatic session tracking is disabled")
	await _wait_for_items("event", 1)
	await get_tree().create_timer(0.2).timeout
	assert_array(_read_items("session")).is_empty()


func test_manual_session_starts_and_ends() -> void:
	SentrySDK.start_session()
	var started: Array[Dictionary] = await _wait_for_items("session", 1)
	if started.is_empty():
		return
	assert_json(started[0]) \
		.must_contain("/init", true) \
		.must_contain("/status", "ok") \
		.verify()
	var sid: String = started[0].sid

	SentrySDK.end_session()
	await _wait_for_items("session", 1,
		func(session: Dictionary) -> bool: return session.status == "exited")

	var sessions: Array[Dictionary] = _read_items("session")
	assert_json(sessions) \
		.with_objects() \
		.must_contain("/sid", sid) \
		.exactly(2)
	assert_json(sessions) \
		.with_objects() \
		.containing("status", "exited") \
		.must_contain("/errors", 0) \
		.exactly(1)


func test_errors_after_end_do_not_update_session() -> void:
	SentrySDK.start_session()
	await _wait_for_items("session", 1)
	SentrySDK.end_session()
	await _wait_for_items("session", 1,
		func(session: Dictionary) -> bool: return session.status == "exited")
	var sessions_before: int = _read_items("session").size()

	push_error("Error outside a session")
	await _wait_for_items("event", 1)
	# Observe a short window for unexpected session updates caused by the error.
	await get_tree().create_timer(0.2).timeout

	var sessions: Array[Dictionary] = _read_items("session")
	assert_int(sessions.size()).is_equal(sessions_before)
	assert_json(sessions) \
		.with_objects() \
		.containing("status", "exited") \
		.must_contain("/errors", 0) \
		.exactly(1)


# Skipped: Cocoa's captureEvent does not increment session errors.
# See https://github.com/getsentry/sentry-godot/issues/1077
func test_start_replaces_session_without_carrying_over_errors(_do_skip = OS.get_name() in ["macOS", "iOS"],
		_skip_reason = "Due to a bug, Godot errors do not increment session errors on Apple platforms.") -> void:
	SentrySDK.start_session()
	var started: Array[Dictionary] = await _wait_for_items("session", 1)
	if started.is_empty():
		return
	var first_sid: String = started[0].sid
	push_error("Error in the replaced session")
	await _wait_for_items("event", 1)

	# New session should replace the old one, and errors should be cleared.
	SentrySDK.start_session()
	var new_sessions: Array[Dictionary] = await _wait_for_items("session", 1,
		func(session: Dictionary) -> bool: return session.sid != first_sid)
	if new_sessions.is_empty():
		return
	var second_sid: String = new_sessions[0].sid
	assert_json(new_sessions[0]) \
		.must_contain("/init", true) \
		.must_contain("/status", "ok") \
		.verify()

	SentrySDK.end_session()
	await _wait_for_items("session", 2,
		func(session: Dictionary) -> bool: return session.status == "exited")

	var sessions: Array[Dictionary] = _read_items("session")
	assert_json(sessions) \
		.with_objects() \
		.containing("status", "exited") \
		.exactly(2)
	assert_json(sessions) \
		.with_objects() \
		.containing("status", "exited") \
		.containing("sid", first_sid) \
		.must_contain("/errors", 1) \
		.exactly(1)
	assert_json(sessions) \
		.with_objects() \
		.containing("status", "exited") \
		.containing("sid", second_sid) \
		.must_contain("/errors", 0) \
		.exactly(1)
