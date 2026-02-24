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

} // namespace sentry::javascript
