extends SentryTestSuite
## Test breadcrumbs interface with detailed JSON validation.


func test_breadcrumbs_with_minimal_breadcrumb() -> void:
	SentrySDK.add_breadcrumb(SentryBreadcrumb.create("Minimal breadcrumb"))

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Very minimal breadcrumb") \
		.at("/breadcrumbs/") \
		.is_array() \
		.with_objects() \
		.containing("message", "Minimal breadcrumb") \

		.either() \
			.must_contain("level", "info") \
		.or_else() \
			.must_not_contain("level") \
		.end() \

		.either() \
			.must_contain("category", "default") \
		.or_else() \
			.must_not_contain("category") \
		.end() \

		.must_not_contain("type") \

		.either() \
			.must_not_contain("data") \
		.or_else() \
			# NOTE: Android always adds data field, even if empty.
			.must_contain("data", {}) \
		.end() \

		.exactly(1)


func test_breadcrumbs_order() -> void:
	SentrySDK.add_breadcrumb(SentryBreadcrumb.create("First breadcrumb"))
	SentrySDK.add_breadcrumb(SentryBreadcrumb.create("Second breadcrumb"))

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	var breadcrumbs: Array = JSON.parse_string(json).get("breadcrumbs", [])
	var ordered_breadcrumbs := breadcrumbs.filter(func(crumb: Dictionary) -> bool:
		return crumb.get("message") in ["First breadcrumb", "Second breadcrumb"]
	)
	assert_array(ordered_breadcrumbs).has_size(2)

	assert_json(ordered_breadcrumbs).describe("First breadcrumb precedes the second breadcrumb") \
		.at("/0") \
		.is_object() \
		.must_contain("message", "First breadcrumb") \
		.verify()
	assert_json(ordered_breadcrumbs).describe("Second breadcrumb follows the first breadcrumb") \
		.at("/1") \
		.is_object() \
		.must_contain("message", "Second breadcrumb") \
		.verify()


func test_breadcrumbs_with_utf8() -> void:
	var crumb := SentryBreadcrumb.create("Hello 世界! 👋")
	crumb.category = "Hello 世界! 👋"
	crumb.type = "Hello 世界! 👋"
	crumb.set_data({"Hello, World! 👋": "Hello 世界! 👋"})
	SentrySDK.add_breadcrumb(crumb)

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Breadcrumb retains UTF-8 encoded data") \
		.at("/breadcrumbs/") \
		.with_objects() \
		.containing("message", "Hello 世界! 👋") \
		.containing("category", "Hello 世界! 👋") \
		.must_contain("type", "Hello 世界! 👋") \
		.must_contain("data", {"Hello, World! 👋": "Hello 世界! 👋"}) \
		.exactly(1)


func test_breadcrumbs_with_complex_nested_data() -> void:
	var crumb := SentryBreadcrumb.create("Player stats updated")
	crumb.category = "gameplay"
	crumb.level = SentrySDK.LEVEL_DEBUG
	crumb.type = "info"
	crumb.set_data({
		"stats": {"health": 85, "inventory": ["sword", "potion", "key"]},
		"level_complete": false,
		"experience_gained": 125.5,
	})
	SentrySDK.add_breadcrumb(crumb)

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Breadcrumb retains complex nested data") \
		.at("/breadcrumbs/") \
		.with_objects() \
		.containing("message", "Player stats updated") \
		.must_contain("category", "gameplay") \
		.must_contain("level", "debug") \
		.must_contain("type", "info") \
		.must_contain("data", {
			"stats": {"health": 85, "inventory": ["sword", "potion", "key"]},
			"level_complete": false,
			"experience_gained": 125.5,
		}) \
		.exactly(1)
