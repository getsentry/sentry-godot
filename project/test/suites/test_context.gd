extends SentryTestSuite
## Test contexts with events.


signal callback_processed


func after_test() -> void:
	SentrySDK._unset_before_send()


func test_set_context_basic() -> void:
	SentrySDK._set_before_send(func (event: SentryEvent) -> SentryEvent:
		var json: String = event.to_json()

		# Verify contexts section exists
		assert_json(json).at("/").must_contain("contexts").verify()

		# Verify our custom gamedev context appears with correct data
		assert_json(json).at("/contexts/game_session") \
			.must_contain("match_id", "match_abc123") \
			.must_contain("game_mode", "battle_royale") \
			.must_contain("players_remaining", 15) \
			.exactly(1)

		callback_processed.emit.call_deferred()
		return null
	)

	SentrySDK.set_context("game_session", {
		"match_id": "match_abc123",
		"game_mode": "battle_royale",
		"players_remaining": 15
	})
	SentrySDK.capture_event(SentrySDK.create_event())

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_set_context_overwrite() -> void:
	SentrySDK._set_before_send(func (event: SentryEvent) -> SentryEvent:
		var json: String = event.to_json()

		# Verify the context was overwritten with new data
		assert_json(json).at("/contexts/game_state") \
			.must_contain("level", "forest_2") \
			.must_contain("score", 2500) \
			.must_not_contain("checkpoint") \
			.exactly(1)

		callback_processed.emit.call_deferred()
		return null
	)

	# Set initial context
	SentrySDK.set_context("game_state", {
		"level": "forest_1",
		"score": 1000,
		"checkpoint": "start"
	})

	# Overwrite with new data
	SentrySDK.set_context("game_state", {
		"level": "forest_2",
		"score": 2500
	})

	SentrySDK.capture_event(SentrySDK.create_event())

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_set_context_complex_data() -> void:
	SentrySDK._set_before_send(func (event: SentryEvent) -> SentryEvent:
		var json: String = event.to_json()

		# Verify nested object structure
		assert_json(json).at("/contexts/player_data/profile") \
			.must_contain("name", "TestPlayer") \
			.must_contain("level", 42) \
			.must_contain("active", true) \
			.exactly(1)

		# Verify array contents
		assert_json(json).at("/contexts/player_data/inventory") \
			.is_array() \
			.must_contain("/0", "sword") \
			.must_contain("/1", "potion") \
			.must_contain("/2", "magic_key") \
			.exactly(1)

		# Verify mixed types in nested structure
		assert_json(json).at("/contexts/player_data/stats") \
			.must_contain("health", 85.5) \
			.must_contain("mana", 60) \
			.must_contain("experience", 1250.75) \
			.exactly(1)

		callback_processed.emit.call_deferred()
		return null
	)

	SentrySDK.set_context("player_data", {
		"profile": {
			"name": "TestPlayer",
			"level": 42,
			"active": true
		},
		"inventory": ["sword", "potion", "magic_key"],
		"stats": {
			"health": 85.5,
			"mana": 60,
			"experience": 1250.75
		}
	})

	SentrySDK.capture_event(SentrySDK.create_event())

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_set_context_edge_cases() -> void:
	SentrySDK._set_before_send(func (event: SentryEvent) -> SentryEvent:
		var json: String = event.to_json()

		# Verify empty context is present
		assert_json(json).at("/contexts/") \
			.must_contain("empty_context", {}) \
			.verify()

		# Verify special characters in context data
		assert_json(json).at("/contexts/special_chars") \
			.must_contain("unicode_text", "Hello 世界! 🚀") \
			.must_contain("symbols", "!@#$%^&*()") \
			.must_contain("quotes", "He said \"Hello\"") \
			.exactly(1)

		# Verify boolean and string values
		assert_json(json).at("/contexts/edge_data") \
			.must_contain("empty_string", "") \
			.must_contain("zero_value", 0) \
			.must_contain("false_value", false) \
			.exactly(1)

		callback_processed.emit.call_deferred()
		return null
	)

	# Test empty context
	SentrySDK.set_context("empty_context", {})

	# Test special characters and Unicode
	SentrySDK.set_context("special_chars", {
		"unicode_text": "Hello 世界! 🚀",
		"symbols": "!@#$%^&*()",
		"quotes": "He said \"Hello\""
	})

	# Test edge values
	SentrySDK.set_context("edge_data", {
		"empty_string": "",
		"zero_value": 0,
		"false_value": false
	})

	SentrySDK.capture_event(SentrySDK.create_event())

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_multiple_contexts_in_single_event() -> void:
	SentrySDK._set_before_send(func (event: SentryEvent) -> SentryEvent:
		var json: String = event.to_json()

		# Verify gameplay context
		assert_json(json).at("/contexts/gameplay") \
			.must_contain("level", "dungeon_3") \
			.must_contain("difficulty", "hard") \
			.must_contain("score", 15750) \
			.exactly(1)

		# Verify player progress context
		assert_json(json).at("/contexts/player_progress") \
			.must_contain("character_level", 25) \
			.must_contain("xp", 45000) \
			.must_contain("gold", 1250) \
			.exactly(1)

		# Verify game settings context
		assert_json(json).at("/contexts/game_settings") \
			.must_contain("graphics_quality", "high") \
			.must_contain("sound_enabled", true) \
			.must_contain("language", "en") \
			.exactly(1)

		callback_processed.emit.call_deferred()
		return null
	)

	# Set multiple different gamedev contexts
	SentrySDK.set_context("gameplay", {
		"level": "dungeon_3",
		"difficulty": "hard",
		"score": 15750
	})

	SentrySDK.set_context("player_progress", {
		"character_level": 25,
		"xp": 45000,
		"gold": 1250
	})

	SentrySDK.set_context("game_settings", {
		"graphics_quality": "high",
		"sound_enabled": true,
		"language": "en"
	})

	SentrySDK.capture_event(SentrySDK.create_event())

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")


func test_default_contexts_presence() -> void:
	SentrySDK._set_before_send(func (event: SentryEvent) -> SentryEvent:
		var json: String = event.to_json()

		# Test standard contexts presence
		assert_json(json).at("/") \
			.must_contain("/contexts/app") \
			.must_contain("/contexts/culture") \
			.must_contain("/contexts/device") \
			.must_contain("/contexts/gpu") \
			.must_contain("/contexts/os") \
			.verify()

		# Test Godot contexts presence
		assert_json(json).at("/") \
			.must_contain("/contexts/godot_engine") \
			.must_contain("/contexts/display") \
			.must_contain("/contexts/environment") \
			.must_contain("/contexts/godot_performance") \
			.verify()

		callback_processed.emit.call_deferred()
		return null
	)

	SentrySDK.capture_event(SentrySDK.create_event())

	var monitor := monitor_signals(self, false)
	await assert_signal(monitor).is_emitted("callback_processed")
