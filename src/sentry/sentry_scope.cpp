#include "sentry_scope.h"

#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

namespace sentry {

void SentryScope::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Contexts cannot have an empty key.");
	contexts[p_key] = p_value;
}

void SentryScope::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Tags cannot have an empty key.");
	tags[p_key] = p_value;
}

void SentryScope::set_user(const Ref<SentryUser> &p_user) {
	user = p_user;
	user_assigned = true;
}

void SentryScope::set_level(sentry::Level p_level) {
	ERR_FAIL_COND_MSG(p_level < LEVEL_DEBUG || p_level > LEVEL_FATAL, "Invalid level.");
	level = p_level;
}

void SentryScope::set_fingerprint(PackedStringArray p_fingerprint) {
	ERR_FAIL_COND_MSG(p_fingerprint.is_empty(), "Fingerprint cannot be empty.");
	fingerprint = p_fingerprint;
}

void SentryScope::set_attribute(const String &p_name, const Variant &p_value) {
	ERR_FAIL_COND_MSG(p_name.is_empty(), "Attribute name cannot be empty.");
	attributes[p_name] = p_value;
}

void SentryScope::add_breadcrumb(const Ref<SentryBreadcrumb> &p_breadcrumb) {
	ERR_FAIL_COND_MSG(p_breadcrumb.is_null(), "Breadcrumb cannot be null.");

	int capacity = SENTRY_OPTIONS()->get_max_breadcrumbs();
	if (capacity <= 0) {
		return;
	}

	if (breadcrumbs.size() < capacity) {
		breadcrumbs.push_back(p_breadcrumb);
	} else {
		breadcrumbs.write[breadcrumbs_next] = p_breadcrumb;
		breadcrumbs_next = (breadcrumbs_next + 1) % breadcrumbs.size();
	}
}

void SentryScope::add_event_processor(const Callable &p_callable) {
	ERR_FAIL_COND_MSG(p_callable.is_null(), "Event processor cannot be null.");
	event_processors.push_back(p_callable);
}

void SentryScope::clear() {
	contexts.clear();
	tags.clear();
	user.unref();
	user_assigned = false;
	level = LEVEL_UNASSIGNED;
	fingerprint.clear();
	attributes.clear();
	breadcrumbs.clear();
	breadcrumbs_next = 0;

	// According to spec, event processors should persist.
}

Ref<SentryScope> SentryScope::clone() const {
	Ref<SentryScope> dup;
	dup.instantiate();

	dup->contexts = contexts.duplicate(true);
	dup->tags = tags.duplicate();
	if (user.is_valid()) {
		dup->user = user->duplicate();
	}
	dup->user_assigned = user_assigned;
	dup->level = level;
	dup->fingerprint = fingerprint; // CoW
	dup->attributes = attributes.duplicate(true);
	dup->breadcrumbs = breadcrumbs; // CoW
	dup->breadcrumbs_next = breadcrumbs_next;
	dup->event_processors = event_processors; // CoW

	return dup;
}

void SentryScope::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_context", "key", "value"), &SentryScope::set_context);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &SentryScope::set_tag);
	ClassDB::bind_method(D_METHOD("set_user", "user"), &SentryScope::set_user);
	ClassDB::bind_method(D_METHOD("set_level", "level"), &SentryScope::set_level);
	ClassDB::bind_method(D_METHOD("set_fingerprint", "fingerprint"), &SentryScope::set_fingerprint);
	ClassDB::bind_method(D_METHOD("set_attribute", "name", "value"), &SentryScope::set_attribute);
	ClassDB::bind_method(D_METHOD("add_breadcrumb", "breadcrumb"), &SentryScope::add_breadcrumb);
	ClassDB::bind_method(D_METHOD("add_event_processor", "callable"), &SentryScope::add_event_processor);
	ClassDB::bind_method(D_METHOD("clear"), &SentryScope::clear);
}

} //namespace sentry
