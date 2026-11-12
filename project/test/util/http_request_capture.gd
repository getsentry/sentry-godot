class_name HTTPRequestCapture extends Node
## Collects SDK HTTP requests through a local server or the Web test runner.

var _server: LocalHTTPServer


func start() -> Error:
	if OS.has_feature("web"):
		if not JavaScriptBridge.eval("Array.isArray(window.__sentryTestRequests)"):
			return ERR_UNAVAILABLE
		JavaScriptBridge.eval("window.__sentryTestRequests.length = 0")
		return OK
	_server = LocalHTTPServer.new()
	add_child(_server)
	return _server.listener.listen(0, "127.0.0.1")


func get_dsn() -> String:
	if OS.has_feature("web"):
		return "https://public@sentry-test.invalid/42"
	return "http://public@127.0.0.1:%d/42" % _server.listener.get_local_port()


## Returns and clears captured requests, with headers and a PackedByteArray body.
func take_requests() -> Array[Dictionary]:
	var requests: Array[Dictionary] = []
	if OS.has_feature("web"):
		var json: String = JavaScriptBridge.eval("JSON.stringify(window.__sentryTestRequests.splice(0))")
		requests.assign(JSON.parse_string(json))
		for request: Dictionary in requests:
			request.body = Marshalls.base64_to_raw(request.body)
	else:
		requests.assign(_server.requests)
		_server.requests.clear()
	return requests


func stop() -> void:
	if is_instance_valid(_server):
		_server.free()
	if OS.has_feature("web"):
		JavaScriptBridge.eval("window.__sentryTestRequests.length = 0")
