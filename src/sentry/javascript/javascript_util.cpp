#include "javascript_util.h"

#include "sentry/util/json_writer.h"

namespace sentry::javascript {

String attributes_to_json(const Dictionary &p_attributes) {
	if (p_attributes.is_empty()) {
		return String();
	}

	util::JSONWriter writer;
	writer.begin_object();

	Array keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		Variant value = p_attributes[key];

		writer.key(key);
		switch (value.get_type()) {
			case Variant::BOOL: {
				writer.value_bool(value.operator bool());
			} break;
			case Variant::INT: {
				writer.value_int(value.operator int64_t());
			} break;
			case Variant::FLOAT: {
				writer.value_float(value.operator double());
			} break;
			case Variant::STRING:
			case Variant::STRING_NAME: {
				writer.value_string(value.operator String());
			} break;
			default: {
				// Stringify other types
				writer.value_string(value.stringify());
			} break;
		}
	}

	writer.end_object();
	return writer.get_string();
}

Variant sentry_js_object_get_attribute(const JSObjectPtr &p_object, const String &p_name) {
	ERR_FAIL_COND_V(!p_object, Variant());

	JSObjectPtr attributes_obj = p_object->get("attributes").as_object();
	if (!attributes_obj) {
		return Variant();
	}

	JSValue attr_val = attributes_obj->get(p_name.utf8());

	// Attributes can be stored either as a typed object { value, type } or as a raw primitive.
	JSObjectPtr attr_obj = attr_val.as_object();
	if (attr_obj) {
		// Typed attribute - return the underlying value.
		return attr_obj->get("value").as_variant();
	}

	// Raw primitive value.
	switch (attr_val.get_type()) {
		case JSValueType::BOOL: {
			return attr_val.as_bool();
		} break;
		case JSValueType::INT: {
			return attr_val.as_int();
		} break;
		case JSValueType::DOUBLE: {
			return attr_val.as_double();
		} break;
		case JSValueType::STRING: {
			return attr_val.as_string();
		} break;
		default: {
			return Variant();
		}
	}
}

void sentry_js_object_set_attribute(const JSObjectPtr &p_object, const String &p_name, const Variant &p_value) {
	ERR_FAIL_COND(!p_object);

	JSObjectPtr attr_obj = p_object->get_or_create_object_property("attributes");
	ERR_FAIL_COND(!attr_obj);

	switch (p_value.get_type()) {
		case Variant::Type::BOOL: {
			attr_obj->set(p_name.utf8(), p_value.operator bool());
		} break;
		case Variant::Type::INT: {
			attr_obj->set(p_name.utf8(), p_value.operator int64_t());
		} break;
		case Variant::Type::FLOAT: {
			attr_obj->set(p_name.utf8(), p_value.operator double());
		} break;
		case Variant::Type::STRING: {
			attr_obj->set(p_name.utf8(), p_value.operator String().utf8());
		} break;
		default: {
			attr_obj->set(p_name.utf8(), p_value.stringify().utf8());
		}
	}
}

} // namespace sentry::javascript
