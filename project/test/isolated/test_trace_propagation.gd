extends SentryTestSuite
## Verifies trace header filtering and the W3C traceparent option.


# TODO: drop the skip when Cocoa gains span support.
func before(_do_skip = OS.get_name() in ["macOS", "iOS"],
		_skip_reason = "Spans are not implemented on this platform yet.") -> void:
	super()


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.traces_sample_rate = 1.0
		# Spelled in mixed case to prove the match ignores case.
		options.trace_propagation_targets = ["API.Example.com"]
		options.propagate_traceparent = true
	)


func _header_value(headers: PackedStringArray, name: String) -> String:
	for line in headers:
		if line.begins_with(name + ": "):
			return line.substr(name.length() + 2)
	return ""


func test_matching_url_receives_headers() -> void:
	var span := SentrySDK.start_span("test.targets_match")
	var headers := span.get_trace_headers("https://api.example.com/scores")
	span.end()

	assert_array(headers) \
		.override_failure_message("a URL matching trace_propagation_targets should receive headers") \
		.is_not_empty()


func test_non_matching_url_receives_nothing() -> void:
	var span := SentrySDK.start_span("test.targets_no_match")
	var headers := span.get_trace_headers("https://third-party.example.org/ads")
	span.end()

	assert_array(headers) \
		.override_failure_message("a URL outside trace_propagation_targets should receive no headers") \
		.is_empty()


func test_omitting_the_url_skips_the_allowlist() -> void:
	var span := SentrySDK.start_span("test.targets_no_url")
	var headers := span.get_trace_headers()
	span.end()

	assert_array(headers) \
		.override_failure_message("reading headers without a URL should skip the allowlist check") \
		.is_not_empty()


func test_traceparent_is_emitted_when_enabled() -> void:
	var span := SentrySDK.start_span("test.traceparent")
	var headers := span.get_trace_headers()
	var json: String = await capture_event_and_get_json(SentrySDK.create_event())
	span.end()

	var data: Variant = JSON.parse_string(json)
	var trace: Dictionary = data.get("contexts", {}).get("trace", {})

	assert_str(_header_value(headers, "traceparent")) \
		.override_failure_message("traceparent should follow the W3C format and carry the span's own ids") \
		.is_equal("00-%s-%s-01" % [trace.get("trace_id"), trace.get("span_id")])
