#include "android_metric.h"

#include "android_string_names.h"

namespace sentry::android {

String AndroidMetric::get_name() const {
	ERR_FAIL_NULL_V(android_plugin, String());
	return android_plugin->call(ANDROID_SN(metricGetName), handle);
}

void AndroidMetric::set_name(const String &p_name) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(metricSetName), handle, p_name);
}

SentryMetric::MetricType AndroidMetric::get_type() const {
	ERR_FAIL_NULL_V(android_plugin, MetricType::METRIC_COUNTER);

	String result = android_plugin->call(ANDROID_SN(metricGetType), handle);
	if (result == "counter") {
		return MetricType::METRIC_COUNTER;
	} else if (result == "gauge") {
		return MetricType::METRIC_GAUGE;
	} else if (result == "distribution") {
		return MetricType::METRIC_DISTRIBUTION;
	} else {
		ERR_PRINT("Sentry: Internal error: Unknown metric type: " + result);
		return MetricType::METRIC_COUNTER;
	}
}

void AndroidMetric::set_type(MetricType p_type) {
	ERR_FAIL_NULL(android_plugin);
	String type = "";
	switch (p_type) {
		case MetricType::METRIC_COUNTER: {
			type = "counter";
		} break;
		case MetricType::METRIC_GAUGE: {
			type = "gauge";
		} break;
		case MetricType::METRIC_DISTRIBUTION: {
			type = "distribution";
		} break;
		default: {
			ERR_PRINT("Sentry: Internal error: Unexpected metric type: " + String::num_int64(p_type));
			return;
		}
	}
	android_plugin->call(ANDROID_SN(metricSetType), handle, type);
}

double AndroidMetric::get_value() const {
	ERR_FAIL_NULL_V(android_plugin, 0.0);
	return android_plugin->call(ANDROID_SN(metricGetValue), handle);
}

void AndroidMetric::set_value(double p_value) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(metricSetValue), handle, p_value);
}

String AndroidMetric::get_unit() const {
	ERR_FAIL_NULL_V(android_plugin, "");
	return android_plugin->call(ANDROID_SN(metricGetUnit), handle);
}

void AndroidMetric::set_unit(const String &p_unit) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(metricSetUnit), handle, p_unit);
}

Variant AndroidMetric::get_attribute(const String &p_name) const {
	ERR_FAIL_NULL_V(android_plugin, Variant());
	Dictionary result = android_plugin->call(ANDROID_SN(metricGetAttribute), handle, p_name);
	return result["value"];
}

void AndroidMetric::set_attribute(const String &p_name, const Variant &p_value) {
	ERR_FAIL_NULL(android_plugin);

	Dictionary attr_data;
	attr_data["name"] = p_name;

	switch (p_value.get_type()) {
		case Variant::BOOL: {
			attr_data["value"] = p_value;
			attr_data["type"] = "boolean";
		} break;
		case Variant::INT: {
			attr_data["value"] = p_value;
			attr_data["type"] = "integer";
		} break;
		case Variant::FLOAT: {
			attr_data["value"] = p_value;
			attr_data["type"] = "double";
		} break;
		default: {
			attr_data["value"] = p_value.stringify();
			attr_data["type"] = "string";
		} break;
	}

	android_plugin->call(ANDROID_SN(metricSetAttribute), handle, attr_data);
}

void AndroidMetric::add_attributes(const Dictionary &p_attributes) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(metricAddAttributes), handle, p_attributes);
}

void AndroidMetric::remove_attribute(const String &p_name) {
	ERR_FAIL_NULL(android_plugin);
	android_plugin->call(ANDROID_SN(metricRemoveAttribute), handle, p_name);
}

AndroidMetric::AndroidMetric() {
	// NOTE: Required for ClassDB registration and compilation to succeed.
	ERR_PRINT("This constructor is not intended for runtime use.");
}

AndroidMetric::AndroidMetric(Object *p_android_plugin, int32_t p_handle) :
		android_plugin(p_android_plugin), handle(p_handle) {
}

AndroidMetric::~AndroidMetric() {
	if (android_plugin && !is_borrowed) {
		android_plugin->call(ANDROID_SN(releaseMetric), handle);
	}
}

} // namespace sentry::android
