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

// Returns type of return value as int or error as negative number.
int32_t object_get(int32_t p_object_id, const char* p_property, sentry::javascript::JSValue *r_ret) {
	return MAIN_THREAD_EM_ASM_INT({
		try {
			const bridge = window.SentryBridge;
			const obj = bridge.getObject($0);
			if (obj === null || obj === undefined) {
				return -1;
			}
			const prop = UTF8ToString($1);
			const result = obj[prop];
			if (result == null || result === undefined) {
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
		const void *p_types, int p_len, sentry::javascript::JSValue *r_ret) {
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

// clang-format on

} // namespace em_js

namespace sentry::javascript {

JSObjectPtr JSObject::create(const String &p_name) {
	CharString name = p_name.ascii();
	int32_t id = em_js::create_object(name.get_data());
	if (id == 0) {
		return JSObjectPtr();
	}
	return JSObjectPtr(new JSObject(id));
}

JSObjectPtr JSObject::get_interface(const char *p_name) {
	int id = em_js::get_interface(p_name);
	if (unlikely(id == 0)) {
		return JSObjectPtr();
	}
	return JSObjectPtr(new JSObject(id));
}

JSReturn JSObject::get(const String &p_property) const {
	JSValue val = {};
	CharString prop = p_property.utf8();
	int result = em_js::object_get(id, prop.get_data(), &val);
	if (result < 0) {
		sentry::logging::print_error("JS interop: get() failed for \"", p_property, "\" property.");
		return JSReturn();
	}
	JSValueType t = (JSValueType)result;

	switch (t) {
		case JSValueType::NIL:
			return JSReturn();
		case JSValueType::BOOL:
			return JSReturn(val.b);
		case JSValueType::INT:
			return JSReturn(val.i);
		case JSValueType::DOUBLE:
			return JSReturn(val.d);
		case JSValueType::STRING: {
			String s = String::utf8(val.c);
			free((void *)val.c);
			return JSReturn(s);
		}
		case JSValueType::OBJECT:
			if (val.id == 0) {
				return JSReturn();
			}
			return JSReturn(JSObjectPtr(new JSObject(val.id)));
		case JSValueType::BYTES:
			// TODO: implement
			return JSReturn();
		default:
			return JSReturn();
	}
}

void JSObject::_set_impl(const char *p_property, const JSValue *p_value, int p_type) {
	int32_t result = em_js::object_set(id, p_property, p_value, p_type);
	if (result < 0) {
		sentry::logging::print_error("JS interop: set() failed for \"", p_property, "\" property.");
	}
}

JSReturn JSObject::_call_impl(const char *p_method, const int *p_types, const JSValue *p_values, int p_len) {
	JSValue val = {};
	int result = em_js::call_method(id, p_method, p_values, p_types, p_len, &val);
	if (result < 0) {
		sentry::logging::print_error("JS interop failed calling \"", p_method, "\" method.");
		return JSReturn();
	}
	JSValueType t = (JSValueType)result;

	switch (t) {
		case JSValueType::NIL:
			return JSReturn();
		case JSValueType::BOOL:
			return JSReturn(val.b);
		case JSValueType::INT:
			return JSReturn(val.i);
		case JSValueType::DOUBLE:
			return JSReturn(val.d);
		case JSValueType::STRING: {
			String s = String::utf8(val.c);
			free((void *)val.c);
			return JSReturn(s);
		}
		case JSValueType::OBJECT:
			if (val.id == 0) {
				return JSReturn();
			}
			return JSReturn(JSObjectPtr(new JSObject(val.id)));
		case JSValueType::BYTES:
			// TODO: implement
			return JSReturn();
		default:
			return JSReturn();
	}
}

JSObject::~JSObject() {
	if (id > 0) {
		em_js::release_object(id);
	}
}

} // namespace sentry::javascript
