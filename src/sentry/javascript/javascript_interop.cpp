#include "javascript_interop.h"

#include "sentry/logging/print.h"

#include <emscripten.h>

// JS interop functions - always run on the main browser thread.
namespace em_js {

// clang-format off

// Create a new JavaScript object and register it with Sentry bridge, returning
// non-zero object ID or 0 on error.
// NOTE: We don't expect UTF8 names in our classes.
int32_t create_object(const char *p_name) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			var n = UTF8ToString($0);
			var ctor = window[n];
			if (!ctor) {
				return 0;
			}
			var obj = new ctor();
			return window.SentryBridge.registerObject(obj);
		} catch (e) {
			console.error("Sentry: JS interop failure:", e);
			return 0;
		}
	}, p_name);
}

void release_object(int32_t p_object_id) {
	MAIN_THREAD_EM_ASM({
		try {
			const bridge = window.SentryBridge;
			bridge.releaseObject($0);
		} catch (e) {
			console.error("Sentry: JS interop: Failed to release object:", e);
		}
	}, p_object_id);
}

// Get interface by name, returning non-zero object ID or 0 on error.
int get_interface(const char *p_name) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const obj = window[UTF8ToString($0)];
			if (obj === null || obj === undefined) {
				return 0;
			}
			return window.SentryBridge.registerObject(obj);
		} catch (e) {
			console.error("Sentry: JS interop: Failed to get interface:", e);
			return 0;
		}
	}, p_name);
}

// Create a callback function from a WASM function pointer, returning non-zero callable ID or 0 on error.
int create_callback(uintptr_t p_func_ptr) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const wasmFunc = wasmTable.get($0);
			if (!wasmFunc) {
				return 0;
			}
			const callback = function() {
				const argc = arguments.length;
				if (argc === 0) {
					wasmFunc(0, 0);
					return;
				}
				const idsPtr = _malloc(argc * 4);
				for (let i = 0; i < argc; i++) {
					HEAP32[(idsPtr >> 2) + i] = window.SentryBridge.registerObject(arguments[i]);
				}
				wasmFunc(idsPtr, argc);
				_free(idsPtr);
			};
			return window.SentryBridge.registerObject(callback);
		} catch (e) {
			console.error("Sentry: JS interop: Failed to create WASM callback:", e);
			return 0;
		}
	}, p_func_ptr);
}

// Returns type of return value as int or error as negative number.
int32_t object_get(int32_t p_object_id, const char* p_property, MarshalData *r_ret) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const bridge = window.SentryBridge;
			const obj = bridge.getObject($0);
			if (obj === null || obj === undefined) {
				return -1;
			}
			const prop = UTF8ToString($1);
			const result = obj[prop];
			if (result === null || result === undefined) {
				return 0;
			}
			const retPtr = $2;
			const type = typeof (result);
			if (type === "boolean") {
				HEAP64[retPtr >> 3] = BigInt(result ? 1 : 0);
				return 1;
			}
			if (type === "number") {
				if (Number.isInteger(result)) {
					HEAP64[retPtr >> 3] = BigInt(result);
					return 2;
				} else {
					HEAPF64[retPtr >> 3] = result;
					return 3;
				}
			}
			if (type === "string") {
				const ptr = stringToNewUTF8(result);
				HEAP64[retPtr >> 3] = BigInt(ptr);
				return 4;
			}
			if (type === "object" || type === "function") {
				HEAP64[retPtr >> 3] = BigInt(bridge.registerObject(result));
				return 5;
			}
			return 0;
		} catch (e) {
			console.error("Sentry JS interop: object_get() failed:", e);
			return -2;
		}
	}, p_object_id, p_property, r_ret);
}

int32_t object_set(int32_t p_object_id, const char *p_property, const void *p_value, const int32_t p_type) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const bridge = window.SentryBridge;
			const obj = bridge.getObject($0);
			if (obj === null || obj === undefined) {
				return -1;
			}
			const prop = UTF8ToString($1);
			const valuePtr = $2;
			const type = $3;
			switch (type) {
				case 0: obj[prop] = null; break;
				case 1: obj[prop] = Boolean(HEAP64[(valuePtr >> 3)]); break;
				case 2: obj[prop] = Number(HEAP64[(valuePtr >> 3)]); break;
				case 3: obj[prop] = HEAPF64[(valuePtr >> 3)]; break;
				case 4: obj[prop] = UTF8ToString(Number(HEAP64[(valuePtr >> 3)])); break;
				case 5: obj[prop] = bridge.getObject(Number(HEAP64[(valuePtr >> 3)])); break;
			}
		} catch (e) {
			console.error("Sentry JS interop: object_set() failed:", e);
			return -2;
		}
		return 0;
	}, p_object_id, p_property, p_value, p_type);
}

// Returns type of return value as int or error as negative number.
int32_t call_method(int32_t p_object_id, const char *p_method, const void *p_args,
		const void *p_types, int p_len, MarshalData *r_ret) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const bridge = window.SentryBridge;
			const obj = bridge.getObject($0);
			if (obj === null || obj === undefined) {
				return -1;
			}
			const method = UTF8ToString($1);
			const valuesPtr = $2;
			const typesPtr = $3;
			const len = $4;
			const args = [];
			for (let i = 0; i < len; i++) {
				const t = HEAP32[(typesPtr >> 2) + i];
				switch (t) {
					case 0: args.push(null); break;
					case 1: args.push(Boolean(HEAP64[(valuesPtr >> 3) + i])); break;
					case 2: args.push(Number(HEAP64[(valuesPtr >> 3) + i])); break;
					case 3: args.push(HEAPF64[(valuesPtr >> 3) + i]); break;
					case 4: args.push(UTF8ToString(Number(HEAP64[(valuesPtr >> 3) + i]))); break;
					case 5: args.push(bridge.getObject(Number(HEAP64[(valuesPtr >> 3) + i]))); break;
				}
			}
			const result = obj[method].apply(obj, args);
			if (result === null || result === undefined) {
				return 0;
			}
			const retPtr = $5;
			const type = typeof (result);
			if (type === "boolean") {
				HEAP64[retPtr >> 3] = BigInt(result ? 1 : 0);
				return 1;
			}
			if (type === "number") {
				if (Number.isInteger(result)) {
					HEAP64[retPtr >> 3] = BigInt(result);
					return 2;
				} else {
					HEAPF64[retPtr >> 3] = result;
					return 3;
				}
			}
			if (type === "string") {
				const ptr = stringToNewUTF8(result);
				HEAP64[retPtr >> 3] = BigInt(ptr);
				return 4;
			}
			if (type === "object" || type === "function") {
				HEAP64[retPtr >> 3] = BigInt(bridge.registerObject(result));
				return 5;
			}
			return 0;
		} catch (e) {
			console.error("Sentry JS interop: call_method() failed:", e);
			return -2;
		}
	}, p_object_id, p_method, p_args, p_types, p_len, r_ret);
}

int object_delete_property(int p_object_id, const char *p_property) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const bridge = window.SentryBridge;
			const obj = bridge.getObject($0);
			if (obj === null || obj === undefined) {
				return -1;
			}
			const prop = UTF8ToString($1);
			delete obj[prop];
		} catch (e) {
			console.error("Sentry JS interop: object_delete_property() failed:", e);
			return -2;
		}
		return 0;
	}, p_object_id, p_property);
}

int object_merge_properties_from_json(int p_object_id, const char *p_json) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const bridge = window.SentryBridge;
			const obj = bridge.getObject($0);
			if (obj === null || obj === undefined) {
				return -1;
			}
			const json = UTF8ToString($1);
			const parsedJson = JSON.parse(json);
			Object.assign(obj, parsedJson);
		} catch (e) {
			return -2;
		}
		return 0;
	}, p_object_id, p_json);
}

int object_push_element_from_json(int p_object_id, const char *p_json) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const bridge = window.SentryBridge;
			const obj = bridge.getObject($0);
			if (obj === null || obj === undefined) {
				return -1;
			}
			const json = UTF8ToString($1);
			const item = JSON.parse(json);
			if (item !== null) {
				obj.push(item);
			}
		} catch (e) {
			console.error("Sentry JS interop: object_push_element_from_json() failed:", e);
			return -2;
		}
		return 0;
	}, p_object_id, p_json);
}

// clang-format on

} // namespace em_js

namespace sentry::javascript {

JSObjectPtr JSObject::create(const char *p_type_name) {
	int32_t id = em_js::create_object(p_type_name);
	if (id == 0) {
		return nullptr;
	}
	return JSObjectPtr(new JSObject(id));
}

JSObjectPtr JSObject::from_id(int32_t p_id) {
	if (p_id == 0) {
		return nullptr;
	}
	return JSObjectPtr(new JSObject(p_id));
}

JSObjectPtr JSObject::get_interface(const char *p_name) {
	int id = em_js::get_interface(p_name);
	if (unlikely(id == 0)) {
		return JSObjectPtr();
	}
	return JSObjectPtr(new JSObject(id));
}

JSObjectPtr JSObject::create_callback(WasmCallbackFunc p_func) {
	// In WASM, function pointers are indices into the function table.
	int id = em_js::create_callback(reinterpret_cast<uintptr_t>(p_func));
	return from_id(id);
}

JSValue JSObject::get(const char *p_property) const {
	em_js::MarshalData rv = {};
	int result = em_js::object_get(id, p_property, &rv);
	if (result < 0) {
		sentry::logging::print_error("JS interop: get() failed for \"", p_property, "\" property.");
		return JSValue();
	}
	JSValueType t = (JSValueType)result;

	switch (t) {
		case JSValueType::NIL:
			return JSValue();
		case JSValueType::BOOL:
			return JSValue(rv.b);
		case JSValueType::INT:
			return JSValue(rv.i);
		case JSValueType::DOUBLE:
			return JSValue(rv.d);
		case JSValueType::STRING: {
			String s = String::utf8(rv.c);
			free((void *)rv.c);
			return JSValue(s);
		}
		case JSValueType::OBJECT:
			if (rv.id == 0) {
				return JSValue();
			}
			return JSValue(JSObjectPtr(new JSObject(rv.id)));
		case JSValueType::BYTES:
			// TODO: implement
			return JSValue();
		default:
			return JSValue();
	}
}

Variant JSObject::get_as_variant(const char *p_property) const {
	JSValue val = get(p_property);
	switch (val.get_type()) {
		case JSValueType::NIL:
			return Variant();
		case JSValueType::BOOL:
			return Variant(val.as_bool());
		case JSValueType::INT:
			return Variant(val.as_int());
		case JSValueType::DOUBLE:
			return Variant(val.as_double());
		case JSValueType::STRING: {
			return Variant(val.as_string());
		}
		case JSValueType::OBJECT:
			// TODO: Convert to dictionary
			return Variant();
		case JSValueType::BYTES:
			// TODO: implement
			return Variant();
		default:
			return Variant();
	}
}

String JSObject::get_as_string(const char *p_property, const String &p_default) const {
	JSValue val = get(p_property);
	if (val.get_type() == JSValueType::STRING) {
		return val.as_string();
	}
	return p_default;
}

JSObjectPtr JSObject::get_or_create_object_property(const char *p_property) {
	JSObjectPtr jso = get(p_property).as_object();
	if (!jso) {
		jso = JSObject::create("Object");
		set(p_property, jso);
	}
	return jso;
}

JSObjectPtr JSObject::get_or_create_array_property(const char *p_property) {
	JSObjectPtr jso = get(p_property).as_object();
	if (!jso) {
		jso = JSObject::create("Array");
		set(p_property, jso);
	}
	return jso;
}

void JSObject::_set_impl(const char *p_property, const em_js::MarshalData *p_value, int p_type) {
	int32_t result = em_js::object_set(id, p_property, p_value, p_type);
	if (result < 0) {
		sentry::logging::print_error("JS interop: set() failed for \"", p_property, "\" property.");
	}
}

JSValue JSObject::_call_impl(const char *p_method, const int *p_types, const em_js::MarshalData *p_values, int p_len) {
	em_js::MarshalData rv = {};
	int result = em_js::call_method(id, p_method, p_values, p_types, p_len, &rv);
	if (result < 0) {
		sentry::logging::print_error("JS interop failed calling \"", p_method, "\" method.");
		return JSValue();
	}
	JSValueType t = (JSValueType)result;

	switch (t) {
		case JSValueType::NIL:
			return JSValue();
		case JSValueType::BOOL:
			return JSValue(rv.b);
		case JSValueType::INT:
			return JSValue(rv.i);
		case JSValueType::DOUBLE:
			return JSValue(rv.d);
		case JSValueType::STRING: {
			String s = String::utf8(rv.c);
			free((void *)rv.c);
			return JSValue(s);
		}
		case JSValueType::OBJECT:
			if (rv.id == 0) {
				return JSValue();
			}
			return JSValue(JSObjectPtr(new JSObject(rv.id)));
		case JSValueType::BYTES:
			// TODO: implement
			return JSValue();
		default:
			return JSValue();
	}
}

void JSObject::delete_property(const char *p_property) {
	int result = em_js::object_delete_property(id, p_property);
	if (result < 0) {
		sentry::logging::print_error("JS interop: Failed deleting \"", p_property, "\" property.");
	}
}

void JSObject::merge_properties_from_json(const char *p_json) {
	int result = em_js::object_merge_properties_from_json(id, p_json);
	if (result < 0) {
		sentry::logging::print_error("JS interop: Failed merging properties from JSON.");
	}
}

void JSObject::push_element_from_json(const char *p_json) {
	int result = em_js::object_push_element_from_json(id, p_json);
	if (result < 0) {
		sentry::logging::print_error("JS interop: Failed pushing element from JSON.");
	}
}

String JSObject::to_json() const {
	// Marshal `this` as OBJECT, not int
	int type = JSValueType::OBJECT;
	em_js::MarshalData val = {};
	val.id = id;
	JSValue result = js_bridge()->_call_impl("objectToJson", &type, &val, 1);
	return result.as_string();
}

JSObject::~JSObject() {
	if (id != 0) {
		em_js::release_object(id);
	}
}

JSObjectPtr js_bridge() {
	static JSObjectPtr bridge = JSObject::get_interface("SentryBridge");
	return bridge;
}

} // namespace sentry::javascript
