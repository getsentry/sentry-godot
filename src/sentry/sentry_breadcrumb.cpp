#include "sentry_breadcrumb.h"

#include "sentry/disabled/disabled_breadcrumb.h"
#include "sentry/util/simple_bind.h"
#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

namespace sentry {

Ref<SentryBreadcrumb> SentryBreadcrumb::create(const String &p_message) {
	Ref<SentryBreadcrumb> instance;
	instance.instantiate();
	if (!p_message.is_empty()) {
		instance->set_message(p_message);
	}
	return instance;
}

void SentryBreadcrumb::set_message(const String &p_message) {
	_impl->set_message(p_message);
}

String SentryBreadcrumb::get_message() const {
	return _impl->get_message();
}

void SentryBreadcrumb::set_category(const String &p_category) {
	_impl->set_category(p_category);
}

String SentryBreadcrumb::get_category() const {
	return _impl->get_category();
}

void SentryBreadcrumb::set_level(sentry::Level p_level) {
	ERR_FAIL_COND_MSG(p_level < LEVEL_DEBUG || p_level > LEVEL_FATAL, "Sentry: Breadcrumb level must be between LEVEL_DEBUG and LEVEL_FATAL.");
	_impl->set_level(p_level);
}

sentry::Level SentryBreadcrumb::get_level() const {
	return _impl->get_level();
}

void SentryBreadcrumb::set_type(const String &p_type) {
	_impl->set_type(p_type);
}

String SentryBreadcrumb::get_type() const {
	return _impl->get_type();
}

void SentryBreadcrumb::set_data(const Dictionary &p_data) {
	_impl->set_data(p_data);
}

Ref<SentryTimestamp> SentryBreadcrumb::get_timestamp() {
	return _impl->get_timestamp();
}

SentryBreadcrumb::SentryBreadcrumb() {
	if (SentrySDK *sdk = SentrySDK::get_singleton()) {
		_impl = sdk->get_internal_sdk()->create_breadcrumb();
	}
	if (!_impl) {
		_impl = memnew(DisabledBreadcrumb);
	}
}

SentryBreadcrumb::SentryBreadcrumb(SentryBreadcrumbImpl *p_impl) :
		_impl(p_impl ? p_impl : memnew(DisabledBreadcrumb)) {
}

SentryBreadcrumb::~SentryBreadcrumb() {
	memdelete(_impl);
}

void SentryBreadcrumb::_bind_methods() {
	ClassDB::bind_static_method("SentryBreadcrumb", D_METHOD("create", "message"), &SentryBreadcrumb::create, DEFVAL(String()));

	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, message);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, category);
	BIND_PROPERTY(SentryBreadcrumb, sentry::make_level_enum_property("level"), set_level, get_level);
	BIND_PROPERTY_SIMPLE(SentryBreadcrumb, Variant::STRING, type);

	ClassDB::bind_method(D_METHOD("set_data", "data"), &SentryBreadcrumb::set_data);
	ClassDB::bind_method(D_METHOD("get_timestamp"), &SentryBreadcrumb::get_timestamp);
}

} //namespace sentry
