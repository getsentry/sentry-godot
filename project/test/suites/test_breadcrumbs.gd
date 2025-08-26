extends SentryTestSuite
## Test breadcrumbs interface with events serialized as JSON.


signal callback_processed


func after_test() -> void:
	SentrySDK._unset_before_send()


@warning_ignore("unused_parameter")
func test_breadcrumbs_basic_functionality(timeout := 10000) -> void:
	SentrySDK._set_before_send(func (event: SentryEvent) -> SentryEvent:
		var json: String = event.to_json()

		# Verify data presence
		assert_json(json).at("/").must_contain("breadcrumbs").verify()

		# Verify both breadcrumbs are present
		assert_json(json).at("/breadcrumbs") \
			.is_array() \
			.with_objects() \
			.at_least(2)

		# Verify breadcrumb with full data
		assert_json(json).at("/breadcrumbs") \
			.is_array() \
			.with_objects() \
			.containing("message", "Player collected coin") \
			.must_contain("category", "gameplay") \
			.must_contain("level", "info") \
			.must_contain("type", "collectible") \
			.must_contain("data", {"coin_type": "gold", "level": "forest_1", "player_score": 1250}) \
			.exactly(1)

		# Verify breadcrumb without data
		assert_json(json).at("/breadcrumbs") \
			.is_array() \
			.with_objects() \
			.containing("message", "Scene transition started") \
			.must_contain("category", "scene") \
			.must_contain("level", "debug") \
			.must_contain("type", "transition") \
			.must_not_contain("data") \
			.exactly(1)

		# Verify breadcrumbs order (-1 last, -2 pre-last)
		assert_json(json).at("/breadcrumbs/-1") \
			.is_object() \
			.must_contain("message", "Scene transition started") \
			.exactly(1)
		assert_json(json).at("/breadcrumbs/-2") \
			.is_object() \
			.must_contain("message", "Player collected coin") \
			.exactly(1)

		callback_processed.emit.call_deferred()
		return null
	)

	SentrySDK.add_breadcrumb("Player collected coin", "gameplay", SentrySDK.LEVEL_INFO, "collectible", {"coin_type": "gold", "level": "forest_1", "player_score": 1250})
	SentrySDK.add_breadcrumb("Scene transition started", "scene", SentrySDK.LEVEL_DEBUG, "transition", {})
	SentrySDK.capture_event(SentrySDK.create_event())

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_breadcrumbs_edge_cases() -> void:
	SentrySDK._set_before_send(func (event: SentryEvent) -> SentryEvent:
		var json: String = event.to_json()

		# Verify minimal breadcrumb (empty type, null data)
		assert_json(json).at("/breadcrumbs") \
			.is_array() \
			.with_objects() \
			.containing("message", "Minimal breadcrumb") \
			.must_contain("level", "info") \
			.must_contain("type", "default") \
			.must_not_contain("category") \
			.must_not_contain("data") \
			.exactly(1)

		# Verify breadcrumb with complex nested data
		assert_json(json).at("/breadcrumbs") \
			.is_array() \
			.with_objects() \
			.containing("message", "Player stats updated") \
			.must_contain("data", {
				"stats": {"health": 85, "inventory": ["sword", "potion", "key"]},
				"level_complete": false,
				"experience_gained": 125.5,
			}) \
			.exactly(1)

		callback_processed.emit.call_deferred()
		return null
	)

	SentrySDK.add_breadcrumb("Minimal breadcrumb")
	SentrySDK.add_breadcrumb("Player stats updated", "gameplay", SentrySDK.LEVEL_ERROR, "stats", {
		"stats": {"health": 85, "inventory": ["sword", "potion", "key"]},
		"level_complete": false,
		"experience_gained": 125.5,
	})
	SentrySDK.capture_event(SentrySDK.create_event())

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")
