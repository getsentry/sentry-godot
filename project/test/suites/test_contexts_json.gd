extends SentryTestSuite
## Test contexts interface with JSON validation.


func test_set_context_basic() -> void:
	SentrySDK.set_context("game_session", {
		"match_id": "match_abc123",
		"game_mode": "battle_royale",
		"players_remaining": 15
	})

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Contexts section exists") \
		.at("/").must_contain("contexts").verify()

	assert_json(json).describe("Custom game_session context appears with correct data") \
		.at("/contexts/game_session") \
		.is_object() \
		.must_contain("match_id", "match_abc123") \
		.must_contain("game_mode", "battle_royale") \
		.must_contain("players_remaining", 15) \
		.verify()


func test_set_context_with_complex_data() -> void:
	SentrySDK.set_context("player_data", {
		"profile": {
			"name": "TestPlayer",
			"level": 42,
			"active": true
		},
		"inventory": ["sword", "potion", "magic_key"],
	})

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Verify nested object structure") \
		.at("/contexts/player_data/profile") \
		.is_object() \
		.must_contain("name", "TestPlayer") \
		.must_contain("level", 42) \
		.must_contain("active", true) \
		.verify()

	assert_json(json).describe("Verify array contents") \
		.at("/contexts/player_data/inventory") \
		.is_array() \
		.has_size(3) \
		.must_contain("/0", "sword") \
		.must_contain("/1", "potion") \
		.must_contain("/2", "magic_key") \
		.verify()


func test_empty_context_is_captured() -> void:
	SentrySDK.set_context("empty_context", {})

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Verify empty context is present") \
		.at("/contexts/") \
		.is_object() \
		.must_contain("empty_context", {}) \
		.verify()


func test_set_context_overwrite() -> void:
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

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Context was overwritten with new data") \
		.at("/contexts/game_state") \
		.must_contain("level", "forest_2") \
		.must_contain("score", 2500) \
		.must_not_contain("checkpoint") \
		.verify()


func test_special_characters_in_context() -> void:
	SentrySDK.set_context("special_chars", {
		"unicode_text": "Hello 世界! 🚀",
		"symbols": "!@#$%^&*()",
		"quotes": "He said \"Hello\"",
		"array": ["🎮", "⚔️", "🏆", "世界"],
	})

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Verify special characters in context data") \
		.at("/contexts/special_chars") \
		.is_object() \
		.must_contain("unicode_text", "Hello 世界! 🚀") \
		.must_contain("symbols", "!@#$%^&*()") \
		.must_contain("quotes", "He said \"Hello\"") \
		.verify()

	assert_json(json).describe("Verify special characters in array data") \
		.at("/contexts/special_chars/array") \
		.is_array() \
		.has_size(4) \
		.has_element("🎮") \
		.has_element("⚔️") \
		.has_element("🏆") \
		.has_element("世界") \
		.verify()


func test_edge_values_in_context() -> void:
	SentrySDK.set_context("edge_data", {
		"empty_string": "",
		"zero_value": 0,
		"false_value": false
	})

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Verify edge values in context data") \
		.at("/contexts/edge_data") \
		.is_object() \
		.must_contain("empty_string", "") \
		.must_contain("zero_value", 0) \
		.must_contain("false_value", false) \
		.verify()


func test_multiple_contexts_in_single_event() -> void:
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

	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Verify gameplay context") \
		.at("/contexts/gameplay") \
		.must_contain("level", "dungeon_3") \
		.must_contain("difficulty", "hard") \
		.must_contain("score", 15750) \
		.verify()

	assert_json(json).describe("Verify player progress context") \
		.at("/contexts/player_progress") \
		.must_contain("character_level", 25) \
		.must_contain("xp", 45000) \
		.must_contain("gold", 1250) \
		.verify()

	assert_json(json).describe("Verify game settings context") \
		.at("/contexts/game_settings") \
		.must_contain("graphics_quality", "high") \
		.must_contain("sound_enabled", true) \
		.must_contain("language", "en") \
		.verify()


func test_default_contexts_presence() -> void:
	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("Test standard contexts presence") \
		.at("/") \
		.must_contain("/contexts/app") \
		.must_contain("/contexts/culture") \
		.must_contain("/contexts/device") \
		.must_contain("/contexts/gpu") \
		.verify()

	assert_json(json).describe("Test Godot contexts presence") \
		.at("/") \
		.must_contain("/contexts/godot_engine") \
		.must_contain("/contexts/display") \
		.must_contain("/contexts/environment") \
		.must_contain("/contexts/godot_performance") \
		.verify()


func test_app_context() -> void:
	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("App context structure") \
		.at("/contexts/app") \
		.is_object() \
		.is_not_empty() \
		.must_contain("app_name") \
		.must_contain("app_version") \
		.verify()

# OS context is added later in processing on Web platform; so we have to skip this test.
func test_os_context(_do_skip = OS.get_name() == "Web") -> void:
	var json: String = await capture_event_and_get_json(SentrySDK.create_event())

	assert_json(json).describe("OS context structure") \
		.at("/contexts/os") \
		.is_object() \
		.is_not_empty() \
		.must_contain("name") \
		.must_contain("version") \
		.verify()
