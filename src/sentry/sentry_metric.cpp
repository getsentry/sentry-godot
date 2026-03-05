#include "sentry_metric.h"

#include "sentry/util/simple_bind.h"

namespace sentry {

void SentryMetric::_bind_methods() {
	BIND_PROPERTY(SentryMetric, PropertyInfo(Variant::STRING, "name"), set_name, get_name);
	BIND_PROPERTY(SentryMetric, PropertyInfo(Variant::INT, "type", godot::PROPERTY_HINT_ENUM, "Counter,Gauge,Distribution"), set_type, get_type);
	BIND_PROPERTY(SentryMetric, PropertyInfo(Variant::FLOAT, "value"), set_value, get_value);
	BIND_PROPERTY(SentryMetric, PropertyInfo(Variant::STRING, "unit"), set_unit, get_unit);

	ClassDB::bind_method(D_METHOD("get_attribute", "name"), &SentryMetric::get_attribute);
	ClassDB::bind_method(D_METHOD("set_attribute", "name", "value"), &SentryMetric::set_attribute);
	ClassDB::bind_method(D_METHOD("add_attributes", "attributes"), &SentryMetric::add_attributes);
	ClassDB::bind_method(D_METHOD("remove_attribute", "name"), &SentryMetric::remove_attribute);

	BIND_ENUM_CONSTANT(METRIC_COUNTER);
	BIND_ENUM_CONSTANT(METRIC_GAUGE);
	BIND_ENUM_CONSTANT(METRIC_DISTRIBUTION);
}

} // namespace sentry
