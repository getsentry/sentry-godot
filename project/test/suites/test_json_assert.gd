extends SentryTestSuite
## Tests for JSONAssert class.


var fixture := """
{
	"ship_model": "Lucky 7",
	"id": 42,
	"components": ["mining_laser", "shield_mk1", "afterboosters"],
	"crew": [
		{"name": "Dan", "role": "engineer"},
		{"name": "Mona", "role": "gunner"},
		{"name": "Taras", "role": "pilot"}
	],
	"engine": {
		"max_speed": 200000,
		"type": "warp"
	}
}
"""


func test_basic_navigation() -> void:
	# Root and simple paths
	assert_json(fixture).at("/").verify()
	assert_json(fixture).at("").verify()
	assert_json(fixture).at("/engine").verify()
	assert_json(fixture).at("engine").verify()

	# Nested paths
	assert_json(fixture).at("/engine/type").verify()
	assert_json(fixture).at("/crew/1/name").verify()

	# Path not found
	assert_failure(func() -> void:
		assert_json(fixture).at("/not_found").verify()
	).is_failed()

	assert_failure(func() -> void:
		assert_json(fixture).at("/engine/not_found").verify()
	).is_failed()


func test_absolute_path_navigation() -> void:
	var test_data := '{"user": {"role": "admin", "profile": {"level": 5}}, "settings": {"theme": "dark"}}'

	assert_json(test_data).describe("absolute paths reset to root context") \
		.at("/user/role") \
		.must_be("admin") \
		.at("/settings/theme") \
		.must_be("dark") \
		.verify()

	# Test absolute path navigation after either/or_else
	assert_json(test_data).describe("absolute path navigation after branching") \
		.at("/user/role") \
		.either().must_be("admin") \
		.or_else().must_be("user") \
		.end() \
		.at("/user/profile/level") \
		.must_be(5) \
		.verify()


func test_array_operations() -> void:
	# Array type validation
	assert_json(fixture).at("/crew").is_array().verify()
	assert_json(fixture).at("/components").is_array().verify()

	# is_array failures
	assert_failure(func() -> void:
		assert_json(fixture).at("/engine").is_array().verify()
	).is_failed()

	# with_objects from arrays
	assert_json(fixture).at("/crew").with_objects().exactly(3)

	# with_objects on non-array fails
	assert_failure(func() -> void:
		assert_json(fixture).at("/engine").with_objects().verify()
	).is_failed()


func test_assertions() -> void:
	# must_contain with key only
	assert_json(fixture).must_contain("ship_model").verify()
	assert_json(fixture).must_contain("id").verify()

	# must_contain with key and value
	assert_json(fixture).must_contain("ship_model", "Lucky 7").verify()
	assert_json(fixture).must_contain("id", 42).verify()

	# must_contain failures
	assert_failure(func() -> void:
		assert_json(fixture).must_contain("not_found").verify()
	).is_failed()

	assert_failure(func() -> void:
		assert_json(fixture).must_contain("ship_model", "Firefly").verify()
	).is_failed()

	# must_not_contain
	assert_json(fixture).must_not_contain("not_found").verify()
	assert_json(fixture).must_not_contain("ship_model", "Firefly").verify()

	# must_not_contain failures
	assert_failure(func() -> void:
		assert_json(fixture).must_not_contain("ship_model").verify()
	).is_failed()

	assert_failure(func() -> void:
		assert_json(fixture).must_not_contain("ship_model", "Lucky 7").verify()
	).is_failed()


func test_filtering() -> void:
	# containing filter
	assert_json(fixture) \
		.at("/crew") \
		.with_objects() \
		.containing("name", "Dan") \
		.must_contain("role", "engineer") \
		.exactly(1)

	assert_json(fixture) \
		.at("/crew") \
		.with_objects() \
		.containing("role", "pilot") \
		.must_contain("name", "Taras") \
		.exactly(1)

	# No objects match criteria
	assert_json(fixture) \
		.at("/crew") \
		.with_objects() \
		.containing("name", "Unknown") \
		.exactly(0)

	# matching filter
	assert_json(fixture) \
		.at("/crew") \
		.with_objects() \
		.matching({"name": "Dan", "role": "engineer"}) \
		.exactly(1)

	assert_json(fixture) \
		.at("/crew") \
		.with_objects() \
		.matching({"role": "gunner"}) \
		.must_contain("name", "Mona") \
		.exactly(1)


func test_finalizers() -> void:
	# commit
	assert_json(fixture).verify()

	# exactly
	assert_json(fixture).exactly(1)
	assert_json(fixture).at("/crew").with_objects().exactly(3)

	assert_failure(func() -> void:
		assert_json(fixture).at("/crew").with_objects().exactly(2)
	).is_failed()

	# at_least
	assert_json(fixture).at("/crew").with_objects().at_least(3)
	assert_json(fixture).at("/crew").with_objects().at_least(1)
	assert_json(fixture).at("/crew").with_objects().at_least(0)

	assert_failure(func() -> void:
		assert_json(fixture).at("/crew").with_objects().at_least(4)
	).is_failed()

	# at_most
	assert_json(fixture).at("/crew").with_objects().at_most(3)
	assert_json(fixture).at("/crew").with_objects().at_most(5)

	assert_failure(func() -> void:
		assert_json(fixture).at("/crew").with_objects().at_most(2)
	).is_failed()


func test_chaining() -> void:
	# Complex realistic chain
	assert_json(fixture) \
		.at("/crew") \
		.is_array() \
		.with_objects() \
		.containing("role", "pilot") \
		.must_contain("name", "Taras") \
		.exactly(1)

	# Multiple filters
	assert_json(fixture) \
		.at("/crew") \
		.with_objects() \
		.containing("role", "engineer") \
		.containing("name", "Dan") \
		.exactly(1)

	# Navigation and validation
	assert_json(fixture) \
		.at("/engine") \
		.must_contain("type", "warp") \
		.must_contain("max_speed", 200_000) \
		.must_not_contain("fuel_type") \
		.verify()

	# must_selected in chain
	assert_json(fixture) \
		.at("/crew") \
		.must_selected(1) \
		.with_objects() \
		.must_selected(3) \
		.containing("name", "Dan") \
		.must_selected(1) \
		.verify()


func test_json_strings() -> void:
	var json_string := '{"test": "value", "number": 42}'
	assert_json(json_string) \
		.must_contain("test", "value") \
		.must_contain("number", 42) \
		.verify()

	# Invalid JSON strings
	assert_failure(func() -> void:
		assert_json('invalid json').verify()
	).is_failed()

	assert_failure(func() -> void:
		assert_json('{"invalid": json}').verify()
	).is_failed()

	assert_failure(func() -> void:
		assert_json('{"missing": "quote}').verify()
	).is_failed()

	# Valid JSON with null
	assert_json('{"value": null}') \
		.must_contain("value", null) \
		.verify()


func test_data_types() -> void:
	var test_data := """
	{
		"string_val": "hello world",
		"int_val": 123,
		"float_val": 3.14,
		"bool_val": true,
		"null_val": null,
		"array_val": [1, 2, 3],
		"object_val": {"nested": "value"}
	}
	"""

	# Test different types
	assert_json(test_data) \
		.must_contain("string_val", "hello world") \
		.must_contain("int_val", 123) \
		.must_contain("float_val", 3.14) \
		.must_contain("bool_val", true) \
		.must_contain("null_val", null) \
		.verify()

	# Test nested object
	assert_json(test_data) \
		.at("/object_val") \
		.must_contain("nested", "value") \
		.verify()

	# Test must_not_contain with wrong types
	assert_json(test_data) \
		.must_not_contain("string_val", 56) \
		.must_not_contain("int_val", "123") \
		.must_not_contain("bool_val", "true") \
		.verify()


func test_array_indexing_edge_cases() -> void:
	# Positive indexing
	assert_json(fixture).at("/crew/0") \
		.must_contain("name", "Dan").verify()
	assert_json(fixture).at("/crew/1") \
		.must_contain("name", "Mona").verify()
	assert_json(fixture).at("/crew/2") \
		.must_contain("name", "Taras").verify()

	# Negative indexing
	assert_json(fixture).at("/crew/-1") \
		.must_contain("name", "Taras").verify()
	assert_json(fixture).at("/crew/-2") \
		.must_contain("name", "Mona").verify()
	assert_json(fixture).at("/crew/-3") \
		.must_contain("name", "Dan").verify()

	# Integers in comparison dictionaries
	# NOTE: Numbers in JSON deserialization are all converted to floats.
	assert_json(fixture).at("/") \
		.must_contain("engine", 	{
			"max_speed": 200000,
			"type": "warp"
		}).verify()

	# Out of bounds
	assert_failure(func() -> void:
		assert_json(fixture).at("/crew/3").verify()
	).is_failed()

	assert_failure(func() -> void:
		assert_json(fixture).at("/crew/-4").verify()
	).is_failed()

	# Invalid indices
	assert_failure(func() -> void:
		assert_json(fixture).at("/crew/invalid").verify()
	).is_failed()


func test_path_resolution_edge_cases() -> void:
	# Various path formats work
	assert_json(fixture).at("crew/0/name").verify()
	assert_json(fixture).at("/crew/0/name/").verify()
	assert_json(fixture).at("//crew//0//name//").verify()

	# Empty array and mixed types
	var mixed := [
		"string", 42, true, null,
		{"type": "object", "value": 1}
	]

	assert_json(mixed).at("/0").verify()
	assert_json(mixed).at("/4/type").verify()
	assert_json(mixed).with_objects().containing("type", "object").exactly(1)

	# Nested arrays
	var nested := {"levels": [[1, 2], [3, 4]]}
	assert_json(nested).at("/levels/1/0").verify()


func test_null_vs_not_found() -> void:
	var test_data := {
		"existing_null": null,
		"nested": {
			"null_value": null,
			"string_value": "world"
		}
	}

	# Can find and assert on null values
	assert_json(test_data) \
		.must_contain("existing_null", null) \
		.at("/existing_null") \
		.verify()

	assert_json(test_data).at("/nested/null_value").verify()

	# Non-existent paths fail
	assert_failure(func() -> void:
		assert_json(test_data).must_contain("non_existent").verify()
	).is_failed()

	assert_failure(func() -> void:
		assert_json(test_data).at("/non_existent").verify()
	).is_failed()

	# must_not_contain works with null
	assert_json(test_data) \
		.must_not_contain("existing_null", "not_null") \
		.must_not_contain("non_existent", null) \
		.verify()


func test_error_conditions() -> void:
	# Multiple candidates for at()
	assert_failure(func() -> void:
		assert_json(fixture) \
			.at("/crew") \
			.with_objects() \
			.at("/name") \
			.verify()
	).is_failed()

	# Array indexing on non-arrays
	assert_failure(func() -> void:
		assert_json(fixture).at("/engine/0").verify()
	).is_failed()

	# Deep path that doesn't exist
	assert_failure(func() -> void:
		assert_json(fixture).at("/engine/turbo/boost/level").verify()
	).is_failed()

	# Empty candidates for assertions
	assert_failure(func() -> void:
		assert_json(fixture) \
			.at("/crew") \
			.with_objects() \
			.containing("name", "nonexistent") \
			.must_contain("field") \
			.verify()
	).is_failed()


func test_mixed_arrays() -> void:
	var mixed := ["string", 42, 3.14, true, false, null, {"valid": true}]

	# Access different types
	assert_json(mixed).at("/0").verify()
	assert_json(mixed).at("/1").verify()
	assert_json(mixed).at("/6/valid").verify()

	# with_objects filters correctly
	assert_json(mixed).with_objects().exactly(1)
	assert_json(mixed).with_objects().containing("valid", true).exactly(1)


func test_empty_values() -> void:
	var test_data := {
		"empty_string": "",
		"empty_array": [],
		"empty_object": {},
		"zero": 0,
		"false_val": false
	}

	# Empty values are valid
	assert_json(test_data) \
		.must_contain("empty_string", "") \
		.must_contain("zero", 0) \
		.must_contain("false_val", false) \
		.verify()

	# Empty array operations
	assert_json(test_data).at("/empty_array").is_array().verify()
	assert_json(test_data).at("/empty_array").with_objects().exactly(0)


func test_either_or_else_basic() -> void:
	var test_data := '{"type": "user", "role": "moderator"}'

	assert_json(test_data).describe("Basic two-branch either/or_else - passes on first branch") \
		.at("/type") \
		.either().must_be("user") \
		.or_else().must_be("admin") \
		.end() \
		.verify()

	assert_json(test_data).describe("Basic two-branch either/or_else - passes on second branch") \
		.at("/type") \
		.either().must_be("admin") \
		.or_else().must_be("user") \
		.end() \
		.verify()

	assert_json(test_data).describe("Multiple branches - passes on third option") \
		.at("/role") \
		.either().must_be("admin") \
		.or_else().must_be("user") \
		.or_else().must_be("moderator") \
		.end() \
		.verify()

	assert_failure(func() -> void:
		assert_json(test_data).describe("All branches fail") \
			.at("/type") \
			.either().must_be("admin") \
			.or_else().must_be("guest") \
			.end() \
			.verify()
	).is_failed()


func test_either_or_else_with_filtering() -> void:
	var test_data := {
		"users": [
			{"name": "Alice", "role": "admin", "active": true},
			{"name": "Bob", "role": "user", "active": false},
			{"name": "Charlie", "role": "moderator", "active": true}
		]
	}

	assert_json(test_data).describe("Either/or_else with object filtering") \
		.at("/users") \
		.with_objects() \
		.either() \
			.containing("role", "admin") \
		.or_else() \
			.containing("role", "moderator") \
		.end() \
		.exactly(2)

	assert_json(test_data).describe("Combining candidates from multiple passing branches") \
		.at("/users") \
		.with_objects() \
		.containing("active", true) \
		.either() \
			.containing("role", "admin") \
		.or_else() \
			.containing("role", "moderator") \
		.end() \
		.exactly(2)


func test_either_or_else_chaining() -> void:
	var test_data := '{"user": {"role": "admin", "level": 5}}'

	assert_json(test_data).describe("Method chaining continues after either/or_else/end") \
		.at("/user/role") \
		.either().must_be("admin") \
		.or_else().must_be("user") \
		.end() \
		.at("/user/level") \
		.must_be(5) \
		.verify()

	assert_json(test_data).describe("Mixed assertion types in branches") \
		.at("/user/level") \
		.either() \
			.is_string() \
		.or_else() \
			.is_number() \
			.must_be(5) \
		.end() \
		.verify()


func test_either_or_else_errors() -> void:
	var test_data := '{"test": "value"}'

	assert_failure(func() -> void:
		assert_json(test_data).describe("or_else without either should fail") \
			.at("/test") \
			.or_else().must_be("other") \
			.end() \
			.verify()
	).is_failed()

	assert_failure(func() -> void:
		assert_json(test_data).describe("end without either should fail") \
			.at("/test") \
			.end() \
			.verify()
	).is_failed()


func test_must_begin_with() -> void:
	var test_data := '{"url": "https://api.example.com", "empty": "", "name": "John Doe"}'

	# Success cases
	assert_json(test_data).describe("String begins with expected prefix") \
		.at("/url") \
		.must_begin_with("https://") \
		.verify()

	assert_json(test_data).describe("Empty string with empty prefix") \
		.at("/empty") \
		.must_begin_with("") \
		.verify()

	# Failure cases
	assert_failure(func() -> void:
		assert_json(test_data).describe("Wrong prefix should fail") \
			.at("/name") \
			.must_begin_with("Jane") \
			.verify()
	).is_failed()

	assert_failure(func() -> void:
		assert_json(test_data).describe("Non-string should fail type check") \
			.at("/") \
			.must_begin_with("test") \
			.verify()
	).is_failed()


func test_must_end_with() -> void:
	var test_data := '{"file": "config.json", "empty": "", "path": "/api/v1"}'

	# Success cases
	assert_json(test_data).describe("String ends with expected suffix") \
		.at("/file") \
		.must_end_with(".json") \
		.verify()

	assert_json(test_data).describe("Empty string with empty suffix") \
		.at("/empty") \
		.must_end_with("") \
		.verify()

	# Failure cases
	assert_failure(func() -> void:
		assert_json(test_data).describe("Wrong suffix should fail") \
			.at("/path") \
			.must_end_with("/v2") \
			.verify()
	).is_failed()

	assert_failure(func() -> void:
		assert_json(test_data).describe("Non-string should fail type check") \
			.at("/") \
			.must_end_with("test") \
			.verify()
	).is_failed()


func test_must_begin_with_and_must_end_with_chaining() -> void:
	var test_data := '{"url": "https://example.com/api/v1"}'

	# Chaining both methods
	assert_json(test_data).describe("URL starts and ends with expected patterns") \
		.at("/url") \
		.must_begin_with("https://") \
		.must_end_with("/v1") \
		.verify()

	# Failure in chain
	assert_failure(func() -> void:
		assert_json(test_data).describe("Chain should fail on wrong suffix") \
			.at("/url") \
			.must_begin_with("https://") \
			.must_end_with("/v2") \
			.verify()
	).is_failed()
