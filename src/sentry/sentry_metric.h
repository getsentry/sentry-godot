#pragma once

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

namespace sentry {

// Represents Sentry metric item.
class SentryMetric : public RefCounted {
	GDCLASS(SentryMetric, RefCounted);

public:
	enum MetricType {
		METRIC_COUNTER,
		METRIC_GAUGE,
		METRIC_DISTRIBUTION
	};

protected:
	static void _bind_methods();

public:
	virtual String get_name() const = 0;
	virtual void set_name(const String &p_name) = 0;

	virtual MetricType get_type() const = 0;
	virtual void set_type(MetricType p_type) = 0;

	virtual double get_value() const = 0;
	virtual void set_value(double p_value) = 0;

	virtual String get_unit() const = 0;
	virtual void set_unit(const String &p_unit) = 0;

	virtual Variant get_attribute(const String &p_name) const = 0;
	virtual void set_attribute(const String &p_name, const Variant &p_value) = 0;
	virtual void add_attributes(const Dictionary &p_attributes) = 0;
	virtual void remove_attribute(const String &p_name) = 0;

	virtual ~SentryMetric() = default;
};

} // namespace sentry

VARIANT_ENUM_CAST(sentry::SentryMetric::MetricType);
