#pragma once

#include "sentry/level.h"
#include "sentry/sentry_breadcrumb.h"
#include "sentry/sentry_event.h"
#include "sentry/sentry_log.h"
#include "sentry/sentry_metric.h"
#include "sentry/sentry_user.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

namespace sentry {

class SentryScope : public RefCounted {
	GDCLASS(SentryScope, RefCounted);

private:
	static constexpr int LEVEL_UNASSIGNED = -1;

	HashMap<String, Dictionary> contexts; // of String => Variant ...
	HashMap<String, String> tags; // String => String
	bool user_assigned = false;
	Ref<SentryUser> user;
	int level = LEVEL_UNASSIGNED; // sentry::Level + unassigned sentinel
	PackedStringArray fingerprint;
	Dictionary attributes; // String => Variant

	// Ring buffer bounded by max_breadcrumbs.
	// Stores breadcrumbs in chronological order, starting at breadcrumbs_next.
	Vector<Ref<SentryBreadcrumb>> breadcrumbs;
	int breadcrumbs_next = 0;

	Vector<Callable> event_processors;

protected:
	static void _bind_methods();

public:
	void set_context(const String &p_key, const Dictionary &p_value);
	void set_tag(const String &p_key, const String &p_value);
	void set_user(const Ref<SentryUser> &p_user);
	void set_level(sentry::Level p_level);
	void set_fingerprint(PackedStringArray p_fingerprint);
	void set_attribute(const String &p_name, const Variant &p_value);
	void add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb);
	void add_event_processor(const Callable &p_callable);
	void clear();

	// *** Internal API / not exposed to GDScript

	Ref<SentryScope> clone() const;
	void apply_to_event(const Ref<SentryEvent> &p_event) const;
	void apply_to_log(const Ref<SentryLog> &p_log) const;
	void apply_to_metric(const Ref<SentryMetric> &p_metric) const;

	SentryScope() = default;
	~SentryScope() = default;
};

} //namespace sentry
