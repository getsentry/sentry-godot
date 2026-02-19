#pragma once

#include <godot_cpp/variant/variant.hpp>
#include <memory>
#include <utility>

using namespace godot;

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

union JSValue {
	bool b;
	int64_t i;
	double d;
	const char *c;
	int32_t id; // object id or buffer id
};

class JSObject;
using JSObjectPtr = std::shared_ptr<JSObject>;

// Return value from JavaScript functions
class JSReturn {
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
			case JSValueType::STRING:
				reinterpret_cast<const String *>(data.mem)->~String();
				break;
			case JSValueType::OBJECT:
				reinterpret_cast<const JSObjectPtr *>(data.mem)->~JSObjectPtr();
				break;
			default:
				break;
		}
	}

public:
	JSValueType get_type() const { return type; }
	int64_t get_raw() const { return data.i; }

	explicit operator int64_t() const {
		if (type == JSValueType::INT) {
			return data.i;
		}
		return 0;
	}

	explicit operator bool() const {
		if (type == JSValueType::BOOL) {
			return data.b;
		}
		return false;
	}

	explicit operator double() const {
		if (type == JSValueType::DOUBLE) {
			return data.d;
		}
		return 0.0;
	}

	explicit operator String() const {
		if (type == JSValueType::STRING) {
			return *reinterpret_cast<const String *>(data.mem);
		}
		return String();
	}

	explicit operator JSObjectPtr() const {
		if (type == JSValueType::OBJECT) {
			return *reinterpret_cast<const JSObjectPtr *>(data.mem);
		}
		return nullptr;
	}

	void operator=(const JSReturn &p_value) {
		if (unlikely(this == &p_value)) {
			return;
		}
		_destroy();
		type = p_value.type;
		switch (type) {
			case JSValueType::NIL:
			case JSValueType::BOOL:
			case JSValueType::INT:
			case JSValueType::DOUBLE: {
				data = p_value.data;
			} break;
			case JSValueType::STRING: {
				memnew_placement(data.mem, String(*reinterpret_cast<const String *>(p_value.data.mem)));
			} break;
			case JSValueType::OBJECT: {
				new (data.mem) JSObjectPtr(*reinterpret_cast<const JSObjectPtr *>(p_value.data.mem));
			} break;
			case JSValueType::BYTES: {
				//TODO: implement
			} break;
		}
	}

	void operator=(JSReturn &&p_value) {
		if (unlikely(this == &p_value)) {
			return;
		}
		_destroy();
		type = p_value.type;
		data = p_value.data;
		p_value.type = JSValueType::NIL;
	}

	JSReturn(const JSReturn &p_value) :
			type(p_value.type) {
		switch (type) {
			case JSValueType::NIL:
			case JSValueType::BOOL:
			case JSValueType::INT:
			case JSValueType::DOUBLE: {
				data = p_value.data;
			} break;
			case JSValueType::STRING: {
				memnew_placement(data.mem, String(*reinterpret_cast<const String *>(p_value.data.mem)));
			} break;
			case JSValueType::OBJECT: {
				new (data.mem) JSObjectPtr(*reinterpret_cast<const JSObjectPtr *>(p_value.data.mem));
			} break;
			case JSValueType::BYTES: {
				// TODO: implement
			} break;
		}
	}

	JSReturn(JSReturn &&p_value) :
			type(p_value.type), data(p_value.data) {
		p_value.type = JSValueType::NIL;
	}

	JSReturn() :
			type(JSValueType::NIL) {
		data.i = 0;
	}

	explicit JSReturn(bool p_value) :
			type(JSValueType::BOOL) { data.b = p_value; }
	explicit JSReturn(int64_t p_value) :
			type(JSValueType::INT) { data.i = p_value; }
	explicit JSReturn(double p_value) :
			type(JSValueType::DOUBLE) { data.d = p_value; }
	explicit JSReturn(const String &p_value) :
			type(JSValueType::STRING) { memnew_placement(data.mem, String(p_value)); }
	explicit JSReturn(const JSObjectPtr &p_value) :
			type(JSValueType::OBJECT) { new (data.mem) JSObjectPtr(p_value); }

	~JSReturn() {
		_destroy();
	}
};

class JSObject {
private:
	int32_t id;

	JSReturn _call_impl(const char *p_method, const int *p_types, const JSValue *p_values, int p_len);
	void _set_impl(const char *p_property, const JSValue *p_value, int p_type);

	// Store overloads: pack a single argument directly into buffers.
	// See call() below.
	static void _store(int &type, JSValue &jval, bool v) {
		type = JSValueType::BOOL;
		jval.b = v;
	}
	static void _store(int &type, JSValue &jval, int v) {
		type = JSValueType::INT;
		jval.i = v;
	}
	static void _store(int &type, JSValue &jval, double v) {
		type = JSValueType::DOUBLE;
		jval.d = v;
	}
	static void _store(int &type, JSValue &jval, const char *v) {
		type = JSValueType::STRING;
		jval.c = v;
	}
	// Should catch String.utf8()/CharString arguments and keep alive for the duration of the call
	static void _store(int &type, JSValue &jval, const CharString &v) {
		type = JSValueType::STRING;
		jval.c = v.get_data();
	}
	static void _store(int &type, JSValue &jval, const JSObjectPtr &v) {
		if (v) {
			type = JSValueType::OBJECT;
			jval.id = v->get_id();
		} else {
			type = JSValueType::NIL;
			jval.i = 0;
		}
	}
	static void _store(int &type, JSValue &jval, const PackedByteArray &v) {
		// TODO: implement
	}
	static void _store(int &type, JSValue &val, std::nullptr_t) {
		type = JSValueType::NIL;
		val.i = 0;
	}

public:
	int32_t get_id() const { return id; };

	JSReturn get(const String &p_property) const;

	template <typename T>
	void set(const String &p_property, const T &p_value) {
		int type = {};
		JSValue val = {};
		_store(type, val, p_value);

		CharString prop = p_property.utf8();
		_set_impl(prop.get_data(), &val, type);
	}

	template <typename... Args>
	JSReturn call(const char *p_method, const Args &...p_args) {
		constexpr int len = sizeof...(p_args);
		// +1 ensures zero-length calls are supported
		int types[len + 1] = {};
		JSValue values[len + 1] = {};

		int idx = 0;
		// Fold expression: executes left-to-right, for each argument in the pack.
		((_store(types[idx], values[idx], p_args), ++idx), ...);

		return _call_impl(p_method, types, values, len);
	}

	static JSObjectPtr create(const String &p_constructor_name);
	// static JSObjectPtr get_interface(const String &p_name);

	// Non-copyable.
	JSObject(const JSObject &) = delete;
	JSObject &operator=(const JSObject &) = delete;

	explicit JSObject(int32_t id) :
			id(id) {}
	JSObject() = delete;
	~JSObject();
};

} // namespace sentry::javascript
