#include "android_metric.h"

#include "android_string_names.h"

namespace sentry::android {

String AndroidMetric::get_name() const {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return String();
	}
	return plugin->call(ANDROID_SN(metricGetName), handle);
}

void AndroidMetric::set_name(const String &p_name) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(metricSetName), handle, p_name);
}

SentryMetric::MetricType AndroidMetric::get_type() const {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return MetricType::METRIC_COUNTER;
	}

	String result = plugin->call(ANDROID_SN(metricGetType), handle);
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
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
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
	plugin->call(ANDROID_SN(metricSetType), handle, type);
}

double AndroidMetric::get_value() const {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return 0.0;
	}
	return plugin->call(ANDROID_SN(metricGetValue), handle);
}

void AndroidMetric::set_value(double p_value) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(metricSetValue), handle, p_value);
}

String AndroidMetric::get_unit() const {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return String();
	}
	return plugin->call(ANDROID_SN(metricGetUnit), handle);
}

void AndroidMetric::set_unit(const String &p_unit) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(metricSetUnit), handle, p_unit);
}

Variant AndroidMetric::get_attribute(const String &p_name) const {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return Variant();
	}
	Dictionary result = plugin->call(ANDROID_SN(metricGetAttribute), handle, p_name);
	return result["value"];
}

void AndroidMetric::set_attribute(const String &p_name, const Variant &p_value) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}

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

	plugin->call(ANDROID_SN(metricSetAttribute), handle, attr_data);
}

void AndroidMetric::add_attributes(const Dictionary &p_attributes) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(metricAddAttributes), handle, p_attributes);
}

void AndroidMetric::remove_attribute(const String &p_name) {
	Object *plugin = _android_plugin.get();
	if (!plugin) {
		return;
	}
	plugin->call(ANDROID_SN(metricRemoveAttribute), handle, p_name);
}

AndroidMetric::AndroidMetric() {
	// NOTE: Required for ClassDB registration and compilation to succeed.
	ERR_PRINT("This constructor is not intended for runtime use.");
}

AndroidMetric::AndroidMetric(util::ObjectHandle<Object> p_android_plugin, int32_t p_handle) :
		_android_plugin(p_android_plugin), handle(p_handle) {
}

AndroidMetric::~AndroidMetric() {
	if (Object *plugin = _android_plugin.get(); plugin && !is_borrowed) {
		plugin->call(ANDROID_SN(releaseMetric), handle);
	}
}

} // namespace sentry::android
