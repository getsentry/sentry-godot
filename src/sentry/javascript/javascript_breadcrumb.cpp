#include "javascript_breadcrumb.h"

#include <godot_cpp/classes/json.hpp>

namespace sentry::javascript {

String JavaScriptBreadcrumb::to_json() const {
	return vformat("{\"type\":\"%s\",\"level\":\"%s\",\"message\":\"%s\",\"category\":\"%s\",\"data\":%s}",
			type, level_as_string(level), message, category, JSON::stringify(data));
}

JavaScriptBreadcrumb::JavaScriptBreadcrumb() {
}

JavaScriptBreadcrumb::~JavaScriptBreadcrumb() {
}

} // namespace sentry::javascript
