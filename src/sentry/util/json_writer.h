#pragma once

#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace sentry::util {

using namespace godot;

// Lightweight JSON writer for efficient JSON string construction.
// Builds JSON directly into a String without intermediate Dictionary/Array allocations.
class JSONWriter {
private:
	String json;
	bool needs_comma = false;

	void _maybe_comma() {
		if (needs_comma) {
			json += ",";
		}
		needs_comma = true;
	}

public:
	JSONWriter() = default;

	// Starts an object: {
	void begin_object() {
		_maybe_comma();
		json += "{";
		needs_comma = false;
	}

	// Ends an object: }
	void end_object() {
		json += "}";
		needs_comma = true;
	}

	// Starts an array: [
	void begin_array() {
		_maybe_comma();
		json += "[";
		needs_comma = false;
	}

	// Ends an array: ]
	void end_array() {
		json += "]";
		needs_comma = true;
	}

	// Writes a key (for objects): "key":
	void key(const String &p_key) {
		_maybe_comma();
		json += "\"" + p_key.json_escape() + "\":";
		needs_comma = false;
	}

	// Writes an escaped string value: "value"
	void value_string(const String &p_value) {
		_maybe_comma();
		json += "\"" + p_value.json_escape() + "\"";
	}

	// Writes an integer value
	void value_int(int64_t p_value) {
		_maybe_comma();
		json += String::num_int64(p_value);
	}

	// Writes a float value
	void value_float(double p_value) {
		_maybe_comma();
		json += String::num(p_value);
	}

	// Writes a boolean value: true or false
	void value_bool(bool p_value) {
		_maybe_comma();
		json += p_value ? "true" : "false";
	}

	// Writes a null value
	void value_null() {
		_maybe_comma();
		json += "null";
	}

	// Writes a Variant value (auto-detects type)
	void value_variant(const Variant &p_value) {
		switch (p_value.get_type()) {
			case Variant::NIL:
				value_null();
				break;
			case Variant::BOOL:
				value_bool(p_value);
				break;
			case Variant::INT:
				value_int(p_value);
				break;
			case Variant::FLOAT:
				value_float(p_value);
				break;
			case Variant::STRING:
			case Variant::STRING_NAME:
				value_string(p_value);
				break;
			default:
				// For complex types, stringify them
				value_string(p_value.stringify());
				break;
		}
	}

	// Writes a key-value pair with string value
	void kv_string(const String &p_key, const String &p_value) {
		key(p_key);
		value_string(p_value);
	}

	// Writes a key-value pair with integer value
	void kv_int(const String &p_key, int64_t p_value) {
		key(p_key);
		value_int(p_value);
	}

	// Writes a key-value pair with float value
	void kv_float(const String &p_key, double p_value) {
		key(p_key);
		value_float(p_value);
	}

	// Writes a key-value pair with boolean value
	void kv_bool(const String &p_key, bool p_value) {
		key(p_key);
		value_bool(p_value);
	}

	// Writes a key-value pair with Variant value
	void kv_variant(const String &p_key, const Variant &p_value) {
		key(p_key);
		value_variant(p_value);
	}

	// Writes a string array as JSON array
	void value_string_array(const PackedStringArray &p_array) {
		begin_array();
		for (int i = 0; i < p_array.size(); i++) {
			value_string(p_array[i]);
		}
		end_array();
	}

	// Writes a key-value pair with string array value
	void kv_string_array(const String &p_key, const PackedStringArray &p_array) {
		key(p_key);
		value_string_array(p_array);
	}

	// Returns the resulting JSON string
	String get_string() const {
		return json;
	}
};

} // namespace sentry::util
