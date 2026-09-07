extends SentryTestSuite
## Verifies that an empty trace_propagation_targets list propagates to nothing.


# TODO: drop the skip when Cocoa gains span support.
func before(_do_skip = OS.get_name() in ["macOS", "iOS"],
		_skip_reason = "Spans are not implemented on this platform yet.") -> void:
	super()


func init_sdk() -> void:
	SentrySDK.init(func(options: SentryOptions) -> void:
		options.traces_sample_rate = 1.0
		options.trace_propagation_targets = []
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
