class_name LocalHTTPServer extends Node
## Local HTTP server for tests that records requests and returns configurable status codes.

## Call listen() to start accepting requests; port 0 selects an available port.
var listener := TCPServer.new()
## Captured requests with path, method, headers (lowercase names), and body (PackedByteArray).
var requests: Array[Dictionary] = []

var _peers: Array[Dictionary] = []


func _process(_delta: float) -> void:
	while listener.is_connection_available():
		_peers.append({
			"stream": listener.take_connection(),
			"buffer": PackedByteArray()
		})

	for peer: Dictionary in _peers.duplicate():
		var stream: StreamPeerTCP = peer.stream
		stream.poll()
		if stream.get_status() != StreamPeerTCP.STATUS_CONNECTED:
			_peers.erase(peer)
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

		# `/status/<code>` responds with the requested HTTP status code.
		var status_code: int = 200
		if path.begins_with("/status/"):
			status_code = path.get_slice("/", 2).to_int()

		# Every path returns the same small response body.
		var body: PackedByteArray = "hello".to_utf8_buffer()
		var response_headers: PackedByteArray = (
			"HTTP/1.1 %d Response\r\nContent-Length: %d\r\nConnection: close\r\n\r\n"
			% [status_code, body.size()]
		).to_utf8_buffer()
		stream.put_data(response_headers)
		stream.put_data(body)
		stream.disconnect_from_host()
		_peers.erase(peer)


func _exit_tree() -> void:
	listener.stop()
	for peer: Dictionary in _peers:
		peer.stream.disconnect_from_host()
