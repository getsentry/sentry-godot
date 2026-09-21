extends SentryTestSuite
## Exercises SentryHTTPRequest end to end through its public API.
## Uses a local TCP server to verify wire requests, trace propagation, and serialized breadcrumbs.
## Complements tests/cpp/tests/test_http_request.cpp, which covers internal lifecycle and span data.

var _server: LocalHTTPServer
var _request: SentryHTTPRequest
var _breadcrumb_start: int = 0


@warning_ignore("unused_parameter")
func before(_do_skip = OS.has_feature("web"),
		_skip_reason = "The HTTP fixture requires a local TCP listener, which Web exports do not support.") -> void:
	_server = LocalHTTPServer.new()
	add_child(_server)
	assert_int(_server.listener.listen(0, "127.0.0.1")).is_equal(OK)
	super()


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.dsn = "http://public@127.0.0.1:%d/42" % _server.listener.get_local_port()
		options.traces_sample_rate = 1.0
		# Only `/propagated/` routes will propagate trace headers.
		options.trace_propagation_targets = ["/propagated/"]
		options.propagate_traceparent = true
		options.godot_logger.enabled = false
	)


func before_test() -> void:
	super()
	SentrySDK.get_current_scope().clear()
	_breadcrumb_start = 0
	var breadcrumbs: Array = await _http_breadcrumbs()
	_breadcrumb_start = breadcrumbs.size()
	_server.requests.clear()
	_request = SentryHTTPRequest.new()
	_request.timeout = 2.0
	add_child(_request)


func after_test() -> void:
	if is_instance_valid(_request):
		_request.free()
	super()


func after() -> void:
	if is_instance_valid(_server):
		_server.free()
	super()


func _url(path: String) -> String:
	return "http://127.0.0.1:%d%s" % [_server.listener.get_local_port(), path]


func _http_breadcrumbs() -> Array:
	var json: String = await capture_event_and_get_json(SentrySDK.create_event())
	var event: Dictionary = JSON.parse_string(json)
	var breadcrumbs: Variant = event.get("breadcrumbs")
	if not breadcrumbs is Array:
		return []
	return breadcrumbs.filter(func(crumb: Dictionary) -> bool:
		return crumb.get("type") == "http").slice(_breadcrumb_start)


func _received(path: String) -> Dictionary:
	var matches: Array = _server.requests.filter(func(item: Dictionary) -> bool:
		return item.path == path)
	assert_array(matches).has_size(1)
	return matches[0]


func test_successful_post_sends_trace_headers_and_preserves_request_data() -> void:
	var request_body := "Hello 世界! 👋"
	var parent: SentrySpan = SentrySDK.start_span("load scores")

	assert_int(_request.request(
		_url("/propagated/ok?token=secret#private"),
		["X-Custom: value"],
		HTTPClient.METHOD_POST,
		request_body,
	)).is_equal(OK)

	var response: Array = await await_signal_on(_request, "request_completed", [], 5000)
	assert_int(response[0]).is_equal(HTTPRequest.RESULT_SUCCESS)
	assert_int(response[1]).is_equal(200)
	assert_str(response[3].get_string_from_utf8()).is_equal("hello")

	var received: Dictionary = _received("/propagated/ok?token=secret")
	assert_str(received.method).is_equal("POST")
	assert_str(received.body.get_string_from_utf8()).is_equal(request_body)
	assert_str(received.headers.get("x-custom", "")).is_equal("value")
	assert_str(received.headers.get("sentry-trace", "")).is_not_empty()
	assert_str(received.headers.get("baggage", "")).contains("sentry-")
	assert_str(received.headers.get("traceparent", "")).starts_with("00-")

	parent.end()


func test_successful_post_records_redacted_breadcrumb() -> void:
	var request_body := "Hello 世界! 👋"

	assert_int(_request.request(
		_url("/propagated/ok?token=secret#private"),
		[],
		HTTPClient.METHOD_POST,
		request_body,
	)).is_equal(OK)
	await await_signal_on(_request, "request_completed", [], 5000)

	var crumbs: Array = await _http_breadcrumbs()
	assert_array(crumbs).has_size(1)
	assert_str(crumbs[0].data.url).is_equal(_url("/propagated/ok"))
	assert_str(crumbs[0].data["http.request.method"]).is_equal("POST")
	assert_int(int(crumbs[0].data["http.request.body.size"])).is_equal(
		request_body.to_utf8_buffer().size()
	)


func test_trace_headers_do_not_duplicate_custom_header_names() -> void:
	var headers := PackedStringArray([
		"Sentry-Trace: caller-trace",
		"Baggage: vendor=value",
		"Traceparent: caller-parent",
	])

	assert_int(_request.request(_url("/propagated/custom"), headers)).is_equal(OK)
	await await_signal_on(_request, "request_completed", [], 5000)

	var received: Dictionary = _received("/propagated/custom").headers
	assert_array(headers).has_size(3)
	assert_str(received["sentry-trace"]).is_equal("caller-trace")
	assert_str(received["baggage"]).is_equal("vendor=value")
	assert_str(received["traceparent"]).is_equal("caller-parent")


func test_url_outside_trace_propagation_targets_keeps_custom_headers_and_omits_trace_headers() -> void:
	# `/not_propagated` is not in `options.trace_propagation_targets`.
	var url: String = _url("/not_propagated")

	assert_int(_request.request(url, ["X-Custom: value"])).is_equal(OK)
	await await_signal_on(_request, "request_completed", [], 5000)

	var received: Dictionary = _received("/not_propagated")
	assert_str(received.headers.get("x-custom", "")).is_equal("value")
	assert_bool(received.headers.has("sentry-trace")).is_false()
	assert_bool(received.headers.has("traceparent")).is_false()
	assert_bool(received.headers.has("baggage")).is_false()


func test_raw_and_empty_request_bodies_keep_exact_byte_counts() -> void:
	var body := PackedByteArray([0, 255, 10, 128])

	assert_int(_request.request_raw(_url("/raw"), [], HTTPClient.METHOD_PUT, body)).is_equal(OK)
	await await_signal_on(_request, "request_completed", [], 5000)

	var received: Dictionary = _received("/raw")
	assert_str(received.method).is_equal("PUT")
	assert_that(received.body).is_equal(body)

	assert_int(_request.request_raw(_url("/empty"), [], HTTPClient.METHOD_POST)).is_equal(OK)
	await await_signal_on(_request, "request_completed", [], 5000)

	received = _received("/empty")
	assert_that(received.body).is_equal(PackedByteArray())

	var crumbs: Array = await _http_breadcrumbs()
	assert_array(crumbs).has_size(2)
	assert_int(int(crumbs[0].data["http.request.body.size"])).is_equal(body.size())
	assert_bool(crumbs[1].data.has("http.request.body.size")).is_false()


func test_connection_failure_records_error_breadcrumb() -> void:
	# Releasing an OS-assigned port gives the request a valid address with no listener.
	var unused_port := TCPServer.new()
	assert_int(unused_port.listen(0, "127.0.0.1")).is_equal(OK)
	var failed_url := "http://127.0.0.1:%d/unreachable" % unused_port.get_local_port()
	unused_port.stop()

	# HTTPRequest reports connection failures asynchronously through request_completed.
	assert_int(_request.request(failed_url)).is_equal(OK)
	var failed_response: Array = await await_signal_on(_request, "request_completed", [], 5000)
	assert_array([
		HTTPRequest.RESULT_CANT_CONNECT,
		HTTPRequest.RESULT_TIMEOUT,
	]).contains([failed_response[0]])

	var crumbs: Array = await _http_breadcrumbs()
	assert_array(crumbs).has_size(1)
	assert_str(crumbs[0].data["error.type"]).is_not_empty()
	assert_str(crumbs[0].level).is_equal("error")


func test_repeated_cancellation_records_one_warning_breadcrumb() -> void:
	assert_int(_request.request(_url("/hold"))).is_equal(OK)
	_request.cancel_request()
	_request.cancel_request()

	var crumbs: Array = await _http_breadcrumbs()
	assert_array(crumbs).has_size(1)
	assert_str(crumbs[0].data["error.type"]).is_equal("cancelled")
	assert_str(crumbs[0].level).is_equal("warning")


class LocalHTTPServer extends Node:
	var listener := TCPServer.new()
	var peers: Array[Dictionary] = []
	var requests: Array[Dictionary] = []

	func _process(_delta: float) -> void:
		while listener.is_connection_available():
			peers.append({
				"stream": listener.take_connection(),
				"buffer": PackedByteArray()
			})

		for peer: Dictionary in peers.duplicate():
			var stream: StreamPeerTCP = peer.stream
			stream.poll()
			if stream.get_status() != StreamPeerTCP.STATUS_CONNECTED:
				peers.erase(peer)
				continue

			var available_bytes: int = stream.get_available_bytes()
			if available_bytes == 0:
				continue

			var read_result: Array = stream.get_data(available_bytes)
			var chunk: PackedByteArray = read_result[1]
			peer.buffer.append_array(chunk)
			var text: String = peer.buffer.get_string_from_ascii()
			var header_end: int = text.find("\r\n\r\n")
			if header_end < 0:
				continue

			var header_lines: PackedStringArray = text.substr(0, header_end).split("\r\n")
			var headers: Dictionary = {}
			for line: String in header_lines.slice(1):
				var colon: int = line.find(":")
				var name: String = line.substr(0, colon).to_lower()
				var value: String = line.substr(colon + 1).strip_edges()
				headers[name] = headers[name] + "," + value if headers.has(name) else value

			var body_start: int = header_end + 4
			var body_length := int(headers.get("content-length", "0"))
			if peer.buffer.size() < body_start + body_length:
				# Try again on the next frame.
				continue

			var request_line: String = header_lines[0]
			var path: String = request_line.get_slice(" ", 1)
			requests.append({
				"path": path,
				"method": request_line.get_slice(" ", 0),
				"headers": headers,
				"body": peer.buffer.slice(body_start, body_start + body_length),
			})

			# `/hold` leaves the request pending so tests can cancel it.
			if path == "/hold":
				continue

			# Every other path returns the same small success response.
			var body: PackedByteArray = "hello".to_utf8_buffer()
			var response_headers: PackedByteArray = (
				"HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\n\r\n" % body.size()
			).to_utf8_buffer()
			stream.put_data(response_headers)
			stream.put_data(body)
			stream.disconnect_from_host()
			peers.erase(peer)

	func _exit_tree() -> void:
		listener.stop()
		for peer: Dictionary in peers:
			peer.stream.disconnect_from_host()
