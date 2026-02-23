#pragma once

#include <godot_cpp/variant/variant.hpp>
#include <memory>

using namespace godot;

namespace em_js {

// Used to pass data across WASM/JS boundary
union MarshalData {
	bool b;
	int64_t i;
	double d;
	const char *c;
	int32_t id; // object id or buffer id
};

} //namespace em_js

namespace sentry::javascript {

enum JSValueType {
	NIL,
	BOOL,
	INT,
	DOUBLE,
	STRING,
	OBJECT,
	BYTES
};

class JSObject;
using JSObjectPtr = std::shared_ptr<JSObject>;

// Return value from JavaScript functions
class JSValue {
private:
	JSValueType type;

	union Data {
		int64_t i;
		bool b;
		double d;
		alignas(String) alignas(JSObjectPtr) uint8_t mem[sizeof(String) > sizeof(JSObjectPtr) ? sizeof(String) : sizeof(JSObjectPtr)];
	} data;

	void _destroy() {
		switch (type) {
			case JSValueType::STRING: {
				reinterpret_cast<const String *>(data.mem)->~String();
			} break;
			case JSValueType::OBJECT: {
				reinterpret_cast<const JSObjectPtr *>(data.mem)->~JSObjectPtr();
			} break;
			default: {
			} break;
		}
	}

public:
	JSValueType get_type() const { return type; }
	int64_t get_raw() const { return data.i; }

	int64_t as_int() const {
		if (type == JSValueType::INT) {
			return data.i;
		}
		return 0;
	}

	bool as_bool() const {
		if (type == JSValueType::BOOL) {
			return data.b;
		}
		return false;
	}

	double as_double() const {
		if (type == JSValueType::DOUBLE) {
			return data.d;
		}
		return 0.0;
	}

	String as_string() const {
		if (type == JSValueType::STRING) {
			return *reinterpret_cast<const String *>(data.mem);
		}
		return String();
	}

	JSObjectPtr as_object() const {
		if (type == JSValueType::OBJECT) {
			return *reinterpret_cast<const JSObjectPtr *>(data.mem);
		}
		return nullptr;
	}

	Variant as_variant() const;

	void operator=(const JSValue &p_other);

	void operator=(JSValue &&p_other) {
		if (unlikely(this == &p_other)) {
			return;
		}
		_destroy();
		data = p_other.data;
		type = p_other.type;
		// Since we're moving data from p_other, we need to ensure that p_value's data is not destroyed
		p_other.type = JSValueType::NIL;
	}

	JSValue(const JSValue &p_other);

	JSValue(JSValue &&p_other) :
			type(p_other.type), data(p_other.data) {
		p_other.type = JSValueType::NIL;
	}

	JSValue() :
			type(JSValueType::NIL) {
		data.i = 0;
	}

	explicit JSValue(bool p_value) :
			type(JSValueType::BOOL) { data.b = p_value; }
	explicit JSValue(int64_t p_value) :
			type(JSValueType::INT) { data.i = p_value; }
	explicit JSValue(double p_value) :
			type(JSValueType::DOUBLE) { data.d = p_value; }
	explicit JSValue(const String &p_value) :
			type(JSValueType::STRING) { memnew_placement(data.mem, String(p_value)); }
	explicit JSValue(const JSObjectPtr &p_value) :
			type(JSValueType::OBJECT) { new (data.mem) JSObjectPtr(p_value); }

	~JSValue() {
		_destroy();
	}
};

// WASM callback signature: receives an array of JS object IDs and the count.
using WasmCallbackFunc = void (*)(int *p_ids, int p_len);

// RAII handle to a JavaScript object, identified by an integer ID.
// Provides property access and method calls across the WASM/JS boundary.
class JSObject {
private:
	int32_t id;

	JSValue _call_impl(const char *p_method, const int *p_types, const em_js::MarshalData *p_values, int p_len);
	void _set_impl(const char *p_property, const em_js::MarshalData *p_value, int p_type);

	// Overloads that marshal a C++ argument into a type tag + MarshalData pair for call() and set().
	static void _store(int &type, em_js::MarshalData &jval, bool v) {
		type = JSValueType::BOOL;
		jval.b = v;
	}
	static void _store(int &type, em_js::MarshalData &jval, int v) {
		type = JSValueType::INT;
		jval.i = v;
	}
	static void _store(int &type, em_js::MarshalData &jval, uint32_t v) {
		type = JSValueType::INT;
		jval.i = v;
	}
	static void _store(int &type, em_js::MarshalData &jval, int64_t v) {
		type = JSValueType::INT;
		jval.i = v;
	}
	static void _store(int &type, em_js::MarshalData &jval, double v) {
		type = JSValueType::DOUBLE;
		jval.d = v;
	}
	static void _store(int &type, em_js::MarshalData &jval, const char *v) {
		type = JSValueType::STRING;
		jval.c = v;
	}
	static void _store(int &type, em_js::MarshalData &jval, const JSObjectPtr &v) {
		if (v) {
			type = JSValueType::OBJECT;
			jval.id = v->get_id();
		} else {
			type = JSValueType::NIL;
			jval.i = 0;
		}
	}
	static void _store(int &type, em_js::MarshalData &jval, const PackedByteArray &v) {
		// TODO: implement
	}
	static void _store(int &type, em_js::MarshalData &val, std::nullptr_t) {
		type = JSValueType::NIL;
		val.i = 0;
	}

public:
	_ALWAYS_INLINE_ int32_t get_id() const { return id; };

	JSValue get(const char *p_property) const;

	JSObjectPtr get_or_create_object_property(const char *p_property);
	JSObjectPtr get_or_create_array_property(const char *p_property);

	template <typename T>
	void set(const char *p_property, const T &p_value) {
		int type = {};
		em_js::MarshalData val = {};
		_store(type, val, p_value);

		_set_impl(p_property, &val, type);
	}

	// Set a string property on a JavaScript object, or remove it if the provided value is empty.
	void set_or_remove_string_property(const char *p_property, const char *p_value) {
		if (p_value == nullptr)
			delete_property(p_property);
		else {
			em_js::MarshalData val = {};
			val.c = p_value;
			_set_impl(p_property, &val, JSValueType::STRING);
		}
	}

	template <typename... Args>
	JSValue call(const char *p_method, const Args &...p_args) {
		constexpr int len = sizeof...(p_args);
		// +1 ensures zero-length calls are supported
		int types[len + 1] = {};
		em_js::MarshalData values[len + 1] = {};

		int idx = 0;
		// Fold expression: executes left-to-right, for each argument in the pack.
		((_store(types[idx], values[idx], p_args), ++idx), ...);

		return _call_impl(p_method, types, values, len);
	}

	void delete_property(const char *p_property);

	// Parse a JSON string into an object and merge its properties into this JavaScript object.
	void merge_properties_from_json(const char *p_json);

	// Parse the provided JSON string, reconstruct it as a JavaScript value, and push it onto this object,
	// assuming this object represents a JavaScript array.
	void push_element_from_json(const char *p_json);

	String to_json() const;

	static JSObjectPtr create(const char *p_type_name);
	static JSObjectPtr from_id(int32_t p_id);
	static JSObjectPtr get_interface(const char *p_name);
	static JSObjectPtr create_callback(WasmCallbackFunc p_func_ptr);

	// Non-copyable.
	JSObject(const JSObject &) = delete;
	JSObject &operator=(const JSObject &) = delete;

	explicit JSObject(int32_t id) :
			id(id) {}
	JSObject() = delete;
	~JSObject();
};

JSObjectPtr js_bridge();

} // namespace sentry::javascript
