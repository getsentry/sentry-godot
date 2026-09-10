extends SentryTestSuite
## Verifies trace header filtering and the W3C traceparent option.


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.traces_sample_rate = 1.0
		# Spelled in mixed case to prove the match ignores case.
		options.trace_propagation_targets = [
			"API.Example.com",
			"literal\\.example\\.com",
			RegEx.create_from_string("^https://regex\\.example\\.com/"),
		]
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


func test_plain_string_with_regex_characters_is_matched_literally() -> void:
	var span := SentrySDK.start_span("test.targets_literal")
	var headers := span.get_trace_headers("https://literal.example.com/scores")
	span.end()

	assert_array(headers) \
		.override_failure_message("regex characters in a string target should remain literal") \
		.is_empty()


func test_regex_target_receives_headers() -> void:
	var span := SentrySDK.start_span("test.targets_regex")
	var headers := span.get_trace_headers("https://regex.example.com/scores")
	span.end()

	assert_array(headers) \
		.override_failure_message("a URL matching a RegEx propagation target should receive headers") \
		.is_not_empty()


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


func test_descendants_never_propagate_empty_ids() -> void:
	var root := SentrySDK.start_span("test.root", {}, null, false)
	var child := SentrySDK.start_span("test.child", {}, root, false)
	assert_array(child.get_trace_headers()).is_not_empty()
	root = null

	var grandchild := SentrySDK.start_span("test.grandchild", {}, child, false)
	var descendant := SentrySDK.start_span("test.descendant", {}, grandchild, false)
	for span: SentrySpan in [grandchild, descendant]:
		var headers := span.get_trace_headers()
		if headers.is_empty():
			continue
		var sentry_trace := _header_value(headers, "sentry-trace")
		assert_str(sentry_trace).is_not_empty().not_contains("0000000000000000")
		assert_str(_header_value(headers, "traceparent")) \
			.is_equal("00-%s-01" % sentry_trace.trim_suffix("-1"))
	descendant.end()
	grandchild.end()
	child.end()


func test_existing_span_headers_survive_ancestor_end() -> void:
	var root := SentrySDK.start_span("test.finished_root", {}, null, false)
	var child := SentrySDK.start_span("test.surviving_child", {}, root, false)
	var before := _header_value(child.get_trace_headers(), "sentry-trace")
	assert_str(before).is_not_empty()
	root.end()

	var headers := child.get_trace_headers()
	assert_str(_header_value(headers, "sentry-trace")).is_equal(before)
	assert_str(_header_value(headers, "traceparent")) \
		.is_equal("00-%s-01" % before.trim_suffix("-1"))
	child.end()
