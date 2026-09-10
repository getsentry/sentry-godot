extends SentryTestSuite
## Verifies that an empty trace_propagation_targets list propagates to nothing.


# TODO: drop the skip when Cocoa gains span support.
func before(_do_skip = OS.get_name() in ["macOS", "iOS"],
		_skip_reason = "Spans are not implemented on this platform yet.") -> void:
	super()


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.traces_sample_rate = 0.0
		options.trace_propagation_targets = []
		options.propagate_traceparent = true
	)


func test_no_url_matches_an_empty_target_list() -> void:
	var span := SentrySDK.start_span("test.targets_empty")
	var headers := span.get_trace_headers("https://api.example.com/scores")
	span.end()

	assert_array(headers) \
		.override_failure_message("an empty trace_propagation_targets list should propagate to nothing") \
		.is_empty()


func test_omitting_the_url_still_yields_headers() -> void:
	var span := SentrySDK.start_span("test.targets_empty_no_url")
	var headers := span.get_trace_headers()
	span.end()

	assert_array(headers) \
		.override_failure_message("reading headers without a URL should skip the allowlist check") \
		.is_not_empty()


func test_unsampled_spans_keep_trace_headers() -> void:
	var root := SentrySDK.start_span("test.unsampled_root", {}, null, false)
	var child := SentrySDK.start_span("test.unsampled_child", {}, root, false)
	for span: SentrySpan in [root, child]:
		var sentry_trace := ""
		var traceparent := ""
		for header: String in span.get_trace_headers():
			if header.begins_with("sentry-trace: "):
				sentry_trace = header.trim_prefix("sentry-trace: ")
			elif header.begins_with("traceparent: "):
				traceparent = header.trim_prefix("traceparent: ")
		assert_str(sentry_trace).ends_with("-0").not_contains("0000000000000000")
		assert_str(traceparent).is_equal("00-%s0" % sentry_trace)
	child.end()
	root.end()
