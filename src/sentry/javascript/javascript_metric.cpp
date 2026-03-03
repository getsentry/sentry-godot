#include "javascript_metric.h"

#include "sentry/javascript/javascript_util.h"

namespace sentry::javascript {

String JavaScriptMetric::get_name() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("name").as_string();
}

void JavaScriptMetric::set_name(const String &p_name) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set("name", p_name.utf8());
}

SentryMetric::MetricType JavaScriptMetric::get_type() const {
	ERR_FAIL_COND_V(!js_obj, SentryMetric::METRIC_COUNTER);
	String type = js_obj->get("type").as_string();
	if (type == "counter") {
		return SentryMetric::METRIC_COUNTER;
	} else if (type == "gauge") {
		return SentryMetric::METRIC_GAUGE;
	} else if (type == "distribution") {
		return SentryMetric::METRIC_DISTRIBUTION;
	} else {
		ERR_PRINT("Sentry: Unexpected metric type: " + type);
		return SentryMetric::METRIC_COUNTER;
	}
}

void JavaScriptMetric::set_type(MetricType p_type) {
	ERR_FAIL_COND(!js_obj);
	switch (p_type) {
		case SentryMetric::METRIC_COUNTER: {
			js_obj->set("type", "counter");
		} break;
		case SentryMetric::METRIC_GAUGE: {
			js_obj->set("type", "gauge");
		} break;
		case SentryMetric::METRIC_DISTRIBUTION: {
			js_obj->set("type", "distribution");
		} break;
		default: {
			ERR_PRINT("Sentry: Unexpected metric type: " + String::num_int64(p_type));
			js_obj->set("type", "counter");
		} break;
	}
}

double JavaScriptMetric::get_value() const {
	ERR_FAIL_COND_V(!js_obj, 0.0);
	return js_obj->get("value").as_double();
}

void JavaScriptMetric::set_value(double p_value) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set("value", p_value);
}

String JavaScriptMetric::get_unit() const {
	ERR_FAIL_COND_V(!js_obj, String());
	return js_obj->get("unit").as_string();
}

void JavaScriptMetric::set_unit(const String &p_unit) {
	ERR_FAIL_COND(!js_obj);
	js_obj->set("unit", p_unit.utf8());
}

Variant JavaScriptMetric::get_attribute(const String &p_name) const {
	return sentry_js_object_get_attribute(js_obj, p_name);
}

void JavaScriptMetric::set_attribute(const String &p_name, const Variant &p_value) {
	sentry_js_object_set_attribute(js_obj, p_name, p_value);
}

void JavaScriptMetric::add_attributes(const Dictionary &p_attributes) {
	ERR_FAIL_COND(!js_obj);

	Array keys = p_attributes.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		set_attribute(key, p_attributes[key]);
	}
}

void JavaScriptMetric::remove_attribute(const String &p_name) {
	ERR_FAIL_COND(!js_obj);

	JSObjectPtr attr_obj = js_obj->get("attributes").as_object();
	if (attr_obj) {
		attr_obj->delete_property(p_name.utf8());
	}
}

JavaScriptMetric::JavaScriptMetric(const JSObjectPtr &p_js_obj) {
	js_obj = p_js_obj;
	ERR_FAIL_COND(!js_obj);
}

JavaScriptMetric::JavaScriptMetric() {
	js_obj = js_bridge()->create("Object");
	ERR_PRINT("This constructor is not intended for runtime use.");
}

JavaScriptMetric::~JavaScriptMetric() {
}

} //namespace sentry::javascript
