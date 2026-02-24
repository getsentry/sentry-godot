#include "javascript_interop.h"

#include "sentry/logging/print.h"

#include <emscripten.h>

// Defines JS interop functions inside C++ unit - always run on the main browser thread.
namespace em_js {

// clang-format off

// Create a new JavaScript object and store it in Sentry bridge, returning
// non-zero object ID or 0 on error.
int32_t create_object(const char *p_name) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const n = UTF8ToString($0);
			const ctor = window[n];
			if (!ctor) {
				return 0;
			}
			const obj = new ctor();
			return window.SentryBridge.storeObject(obj);
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
int32_t get_interface(const char *p_name) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const obj = window[UTF8ToString($0)];
			if (obj === null || obj === undefined) {
				return 0;
			}
			return window.SentryBridge.storeObject(obj);
		} catch (e) {
			console.error("Sentry: JS interop: Failed to get interface:", e);
			return 0;
		}
	}, p_name);
}

// Create a callback function from a WASM function pointer, returning non-zero callable ID or 0 on error.
int32_t create_callback(uintptr_t p_func_ptr) {
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
					HEAP32[(idsPtr >> 2) + i] = window.SentryBridge.storeObject(arguments[i]);
				}
				wasmFunc(idsPtr, argc);
				_free(idsPtr);
			};
			return window.SentryBridge.storeObject(callback);
		} catch (e) {
			console.error("Sentry: JS interop: Failed to create WASM callback:", e);
			return 0;
		}
	}, p_func_ptr);
}

// Gets value of object's property, returning its type code or error as negative number.
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
				HEAP8[retPtr] = result ? 1 : 0;
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
				HEAPU32[retPtr >> 2] = stringToNewUTF8(result);
				return 4;
			}
			if (result instanceof Uint8Array) {
				HEAP32[retPtr >> 2] = bridge.storeBytes(result);
				return 6;
			}
			if (type === "object" || type === "function") {
				HEAP32[retPtr >> 2] = bridge.storeObject(result);
				return 5;
			}
			return 0;
		} catch (e) {
			console.error("Sentry JS interop: object_get() failed:", e);
			return -2;
		}
	}, p_object_id, p_property, r_ret);
}

// Sets value of object's property, returning 0 on success or negative number on error.
int32_t object_set(int32_t p_object_id, const char *p_property, const MarshalData *p_value, sentry::javascript::JSValueType p_type) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const bridge = window.SentryBridge;
			const obj = bridge.getObject($0);
			const valuePtr = $2;
			const type = $3;
			if (obj === null || obj === undefined) {
				// Release stored bytes to prevent leaking in the byte store.
				if (type === 6) {
					bridge.releaseBytes(HEAP32[valuePtr >> 2]);
				}
				return -1;
			}
			const prop = UTF8ToString($1);
			switch (type) {
				case 0: obj[prop] = null; break;
				case 1: obj[prop] = Boolean(HEAP8[valuePtr]); break;
				case 2: obj[prop] = Number(HEAP64[(valuePtr >> 3)]); break;
				case 3: obj[prop] = HEAPF64[(valuePtr >> 3)]; break;
				case 4: obj[prop] = UTF8ToString(HEAPU32[valuePtr >> 2]); break;
				case 5: obj[prop] = bridge.getObject(HEAP32[valuePtr >> 2]); break;
				case 6: obj[prop] = bridge.takeBytes(HEAP32[valuePtr >> 2]); break;
			}
		} catch (e) {
			console.error("Sentry JS interop: object_set() failed:", e);
			return -2;
		}
		return 0;
	}, p_object_id, p_property, p_value, p_type);
}

// Calls object's method, returning RV's type code or negative number on error.
// Each argument is a tagged union (MarshalData) - must be read via the heap view matching its type tag.
int32_t call_method(int32_t p_object_id, const char *p_method, const MarshalData *p_args,
		const sentry::javascript::JSValueType *p_types, int32_t p_len, MarshalData *r_ret) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const bridge = window.SentryBridge;
			const obj = bridge.getObject($0);
			const valuesPtr = $2;
			const typesPtr = $3;
			const len = $4;
			if (obj === null || obj === undefined) {
				// Release any stored bytes to prevent leaking in the byte store.
				for (let i = 0; i < len; i++) {
					if (HEAP32[(typesPtr >> 2) + i] === 6) {
						bridge.releaseBytes(HEAP32[(valuesPtr + i * 8) >> 2]);
					}
				}
				return -1;
			}
			const method = UTF8ToString($1);
			const args = [];
			for (let i = 0; i < len; i++) {
				const t = HEAP32[(typesPtr >> 2) + i];
				const off = valuesPtr + i * 8;
				switch (t) {
					case 0: args.push(null); break;
					case 1: args.push(Boolean(HEAP8[off])); break;
					case 2: args.push(Number(HEAP64[off >> 3])); break;
					case 3: args.push(HEAPF64[off >> 3]); break;
					case 4: args.push(UTF8ToString(HEAPU32[off >> 2])); break;
					case 5: args.push(bridge.getObject(HEAP32[off >> 2])); break;
					case 6: args.push(bridge.takeBytes(HEAP32[off >> 2])); break;
				}
			}
			const result = obj[method].apply(obj, args);
			if (result === null || result === undefined) {
				return 0;
			}
			const retPtr = $5;
			const type = typeof (result);
			if (type === "boolean") {
				HEAP8[retPtr] = result ? 1 : 0;
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
				HEAPU32[retPtr >> 2] = stringToNewUTF8(result);
				return 4;
			}
			if (result instanceof Uint8Array) {
				HEAP32[retPtr >> 2] = bridge.storeBytes(result);
				return 6;
			}
			if (type === "object" || type === "function") {
				HEAP32[retPtr >> 2] = bridge.storeObject(result);
				return 5;
			}
			return 0;
		} catch (e) {
			console.error("Sentry JS interop: call_method() failed:", e);
			return -2;
		}
	}, p_object_id, p_method, p_args, p_types, p_len, r_ret);
}

// Deletes a property from an object, returning 0 on success or negative number on error.
int32_t object_delete_property(int32_t p_object_id, const char *p_property) {
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

// Merges properties from a JSON string into an object, returning 0 on success or negative number on error.
int32_t object_merge_properties_from_json(int32_t p_object_id, const char *p_json) {
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
			console.error("Sentry JS interop: object_merge_properties_from_json() failed:", e);
			return -2;
		}
		return 0;
	}, p_object_id, p_json);
}

// Parses object from JSON string and adds it to an array, returning 0 on success or negative number on error.
int32_t object_push_element_from_json(int32_t p_object_id, const char *p_json) {
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

// Serializes object to JSON string, returning 0 on success or negative number on error.
int32_t object_to_json(int32_t p_object_id, MarshalData *r_ret) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const bridge = window.SentryBridge;
			const obj = bridge.getObject($0);
			if (obj === null || obj === undefined) {
				return -1;
			}
			const json = JSON.stringify(obj);
			if (json == null) {
				return -1;
			}
			const jsonPtr = stringToNewUTF8(json);
			const retPtr = $1;
			HEAPU32[retPtr >> 2] = jsonPtr;
			return 0;
		} catch (e) {
			console.error("Sentry JS interop: object_to_json() failed:", e);
			return -2;
		}
	}, p_object_id, r_ret);
}

int32_t store_bytes(const PackedByteArray &p_bytes) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const dataPtr = $0;
			const size = $1;
			var bytes = new Uint8Array(size);
			bytes.set(HEAPU8.subarray(dataPtr, dataPtr + size));
			return window.SentryBridge.storeBytes(bytes);
		} catch (e) {
			console.error("Sentry JS interop: Failed to store bytes:", e);
			return 0;
		}
	}, p_bytes.ptr(), (int32_t)p_bytes.size());
}

PackedByteArray take_bytes(int32_t p_id) {
	em_js::MarshalData buf = {};
	int32_t size = MAIN_THREAD_EM_ASM_INT({
		try {
			const bytes = window.SentryBridge.takeBytes($0);
			if (!bytes || bytes.byteLength === 0) {
				return 0;
			}
			const size = bytes.byteLength;
			const ptr = _malloc(size);
			HEAPU8.set(bytes, ptr);
			const retPtr = $1;
			HEAPU32[retPtr >> 2] = ptr;
			return size;
		} catch (e) {
			console.error("Sentry JS interop: Failed to retrieve bytes:", e);
			return -1;
		} }, p_id, &buf);
	if (size <= 0) {
		return PackedByteArray();
	}
	PackedByteArray bytes;
	bytes.resize(size);
	memcpy(bytes.ptrw(), buf.c, size);
	free((void *)buf.c);
	return bytes;
}

// clang-format on

} // namespace em_js

namespace {

using namespace sentry::javascript;

JSValue _unmarshal_return_value(JSValueType p_type, em_js::MarshalData &p_data) {
	switch (p_type) {
		case JSValueType::NIL:
			return JSValue();
		case JSValueType::BOOL:
			return JSValue(p_data.b);
		case JSValueType::INT:
			return JSValue(p_data.i);
		case JSValueType::DOUBLE:
			return JSValue(p_data.d);
		case JSValueType::STRING: {
			String s = String::utf8(p_data.c);
			free((void *)p_data.c);
			return JSValue(s);
		}
		case JSValueType::OBJECT:
			if (p_data.id == 0) {
				return JSValue();
			}
			return JSValue(JSObject::from_id(p_data.id));
		case JSValueType::BYTES:
			if (p_data.id == 0) {
				return JSValue(PackedByteArray());
			}
			return JSValue(em_js::take_bytes(p_data.id));
		default:
			return JSValue();
	}
}

} // unnamed namespace

namespace sentry::javascript {

// *** JSValue

Variant JSValue::as_variant() const {
	switch (type) {
		case JSValueType::NIL:
			return Variant();
		case JSValueType::BOOL:
			return Variant(as_bool());
		case JSValueType::INT:
			return Variant(as_int());
		case JSValueType::DOUBLE:
			return Variant(as_double());
		case JSValueType::STRING: {
			return Variant(as_string());
		}
		case JSValueType::OBJECT:
			ERR_PRINT("Not supported - JSObject cannot be stored in Variant.");
			return Variant();
		case JSValueType::BYTES:
			return Variant(as_bytes());
		default:
			return Variant();
	}
}

void JSValue::operator=(const JSValue &p_other) {
	if (unlikely(this == &p_other)) {
		return;
	}
	_destroy();
	type = p_other.type;
	switch (type) {
		case JSValueType::NIL:
		case JSValueType::BOOL:
		case JSValueType::INT:
		case JSValueType::DOUBLE: {
			data = p_other.data;
		} break;
		case JSValueType::STRING: {
			memnew_placement(data.mem, String(*reinterpret_cast<const String *>(p_other.data.mem)));
		} break;
		case JSValueType::OBJECT: {
			new (data.mem) JSObjectPtr(*reinterpret_cast<const JSObjectPtr *>(p_other.data.mem));
		} break;
		case JSValueType::BYTES: {
			memnew_placement(data.mem, PackedByteArray(*reinterpret_cast<const PackedByteArray *>(p_other.data.mem)));
		} break;
	}
}

JSValue::JSValue(const JSValue &p_other) :
		type(p_other.type) {
	switch (type) {
		case JSValueType::NIL:
		case JSValueType::BOOL:
		case JSValueType::INT:
		case JSValueType::DOUBLE: {
			data = p_other.data;
		} break;
		case JSValueType::STRING: {
			memnew_placement(data.mem, String(*reinterpret_cast<const String *>(p_other.data.mem)));
		} break;
		case JSValueType::OBJECT: {
			new (data.mem) JSObjectPtr(*reinterpret_cast<const JSObjectPtr *>(p_other.data.mem));
		} break;
		case JSValueType::BYTES: {
			memnew_placement(data.mem, PackedByteArray(*reinterpret_cast<const PackedByteArray *>(p_other.data.mem)));
		} break;
	}
}

// *** JSObject

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
	int32_t id = em_js::get_interface(p_name);
	if (unlikely(id == 0)) {
		return JSObjectPtr();
	}
	return JSObjectPtr(new JSObject(id));
}

JSObjectPtr JSObject::create_callback(WasmCallbackFunc p_func) {
	// In WASM, function pointers are indices into the function table.
	int32_t id = em_js::create_callback(reinterpret_cast<uintptr_t>(p_func));
	return from_id(id);
}

JSValue JSObject::get(const char *p_property) const {
	em_js::MarshalData rv = {};
	int32_t result = em_js::object_get(id, p_property, &rv);
	if (result < 0) {
		sentry::logging::print_error("JS interop: get() failed for \"", p_property, "\" property.");
		return JSValue();
	}
	return _unmarshal_return_value((JSValueType)result, rv);
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

void JSObject::_set_impl(const char *p_property, const em_js::MarshalData *p_value, JSValueType p_type) {
	int32_t result = em_js::object_set(id, p_property, p_value, p_type);
	if (result < 0) {
		sentry::logging::print_error("JS interop: set() failed for \"", p_property, "\" property.");
	}
}

JSValue JSObject::_call_impl(const char *p_method, const JSValueType *p_types, const em_js::MarshalData *p_values, int32_t p_len) {
	em_js::MarshalData rv = {};
	int32_t result = em_js::call_method(id, p_method, p_values, p_types, p_len, &rv);
	if (result < 0) {
		sentry::logging::print_error("JS interop failed calling \"", p_method, "\" method.");
		return JSValue();
	}
	return _unmarshal_return_value((JSValueType)result, rv);
}

void JSObject::delete_property(const char *p_property) {
	int32_t result = em_js::object_delete_property(id, p_property);
	if (result < 0) {
		sentry::logging::print_error("JS interop: Failed deleting \"", p_property, "\" property.");
	}
}

void JSObject::merge_properties_from_json(const char *p_json) {
	int32_t result = em_js::object_merge_properties_from_json(id, p_json);
	if (result < 0) {
		sentry::logging::print_error("JS interop: Failed merging properties from JSON.");
	}
}

void JSObject::push_element_from_json(const char *p_json) {
	int32_t result = em_js::object_push_element_from_json(id, p_json);
	if (result < 0) {
		sentry::logging::print_error("JS interop: Failed pushing element from JSON.");
	}
}

String JSObject::to_json() const {
	em_js::MarshalData jsonData = {};
	int32_t result = em_js::object_to_json(id, &jsonData);
	if (result < 0) {
		sentry::logging::print_error("JS interop: Failed converting object to JSON.");
		return String();
	}
	String json = String::utf8(jsonData.c);
	free((void *)jsonData.c);
	return json;
}

JSObject::~JSObject() {
	if (id != 0) {
		em_js::release_object(id);
	}
}

JSObjectPtr js_bridge() {
	// Safe to cache: the bridge script is injected at HTML <head>, so SentryBridge
	// is always registered on window before any WASM/C++ code executes.
	static JSObjectPtr bridge = JSObject::get_interface("SentryBridge");
	return bridge;
}

} // namespace sentry::javascript
