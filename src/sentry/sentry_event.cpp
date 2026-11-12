#include "sentry_event.h"

#include "sentry/disabled/disabled_event.h"
#include "sentry/sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

#include <godot_cpp/classes/global_constants.hpp>

namespace sentry {

String SentryEvent::get_id() const {
	return _impl->get_id();
}

void SentryEvent::set_message(const String &p_message) {
	_impl->set_message(p_message);
}

String SentryEvent::get_message() const {
	return _impl->get_message();
}

void SentryEvent::set_timestamp(const Ref<SentryTimestamp> &p_timestamp) {
	_impl->set_timestamp(p_timestamp);
}

Ref<SentryTimestamp> SentryEvent::get_timestamp() const {
	return _impl->get_timestamp();
}

String SentryEvent::get_platform() const {
	return _impl->get_platform();
}

void SentryEvent::set_level(sentry::Level p_level) {
	ERR_FAIL_COND_MSG(p_level < LEVEL_DEBUG || p_level > LEVEL_FATAL, "Sentry: Event level must be between LEVEL_DEBUG and LEVEL_FATAL.");
	_impl->set_level(p_level);
}

sentry::Level SentryEvent::get_level() const {
	return _impl->get_level();
}

void SentryEvent::set_logger(const String &p_logger) {
	_impl->set_logger(p_logger);
}

String SentryEvent::get_logger() const {
	return _impl->get_logger();
}

void SentryEvent::set_release(const String &p_release) {
	_impl->set_release(p_release);
}

String SentryEvent::get_release() const {
	return _impl->get_release();
}

void SentryEvent::set_dist(const String &p_dist) {
	_impl->set_dist(p_dist);
}

String SentryEvent::get_dist() const {
	return _impl->get_dist();
}

void SentryEvent::set_environment(const String &p_environment) {
	_impl->set_environment(p_environment);
}

String SentryEvent::get_environment() const {
	return _impl->get_environment();
}

void SentryEvent::set_tag(const String &p_key, const String &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set tag with an empty key.");
	_impl->set_tag(p_key, p_value);
}

void SentryEvent::remove_tag(const String &p_key) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't remove tag with an empty key.");
	_impl->remove_tag(p_key);
}

String SentryEvent::get_tag(const String &p_key) {
	ERR_FAIL_COND_V_MSG(p_key.is_empty(), String(), "Sentry: Can't get tag with an empty key.");
	return _impl->get_tag(p_key);
}

void SentryEvent::set_user(const Ref<SentryUser> &p_user) {
	_impl->set_user(p_user);
}

void SentryEvent::set_fingerprint(const PackedStringArray &p_fingerprint) {
	_impl->set_fingerprint(p_fingerprint);
}

void SentryEvent::set_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't set context with an empty key.");
	_impl->set_context(p_key, p_value);
}

void SentryEvent::merge_context(const String &p_key, const Dictionary &p_value) {
	ERR_FAIL_COND_MSG(p_key.is_empty(), "Sentry: Can't merge context with an empty key.");
	_impl->merge_context(p_key, p_value);
}

void SentryEvent::add_exception(const Exception &p_exception) {
	_impl->add_exception(p_exception);
}

int SentryEvent::get_exception_count() const {
	return _impl->get_exception_count();
}

void SentryEvent::set_exception_value(int p_index, const String &p_value) {
	_impl->set_exception_value(p_index, p_value);
}

String SentryEvent::get_exception_value(int p_index) const {
	return _impl->get_exception_value(p_index);
}

bool SentryEvent::is_crash() const {
	return _impl->is_crash();
}

String SentryEvent::to_json() const {
	return _impl->to_json();
}

Ref<SentryEvent> SentryEvent::create_noop() {
	SentryEventImpl *impl = memnew(DisabledEvent);
	return Ref<SentryEvent>(memnew(SentryEvent(impl)));
}

SentryEvent::SentryEvent() {
	if (SentrySDK *sdk = SentrySDK::get_singleton()) {
		_impl = sdk->get_internal_sdk()->create_event();
	}
	if (!_impl) {
		_impl = memnew(DisabledEvent);
	}
}

SentryEvent::SentryEvent(SentryEventImpl *p_impl) :
		_impl(p_impl ? p_impl : memnew(DisabledEvent)) {
}

SentryEvent::~SentryEvent() {
	memdelete(_impl);
}

void SentryEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &SentryEvent::get_id);
	ClassDB::bind_method(D_METHOD("set_message", "message"), &SentryEvent::set_message);
	ClassDB::bind_method(D_METHOD("get_message"), &SentryEvent::get_message);
	ClassDB::bind_method(D_METHOD("set_timestamp", "timestamp"), &SentryEvent::set_timestamp);
	ClassDB::bind_method(D_METHOD("get_timestamp"), &SentryEvent::get_timestamp);
	ClassDB::bind_method(D_METHOD("get_platform"), &SentryEvent::get_platform);
	ClassDB::bind_method(D_METHOD("set_level", "level"), &SentryEvent::set_level);
	ClassDB::bind_method(D_METHOD("get_level"), &SentryEvent::get_level);
	ClassDB::bind_method(D_METHOD("set_logger", "logger"), &SentryEvent::set_logger);
	ClassDB::bind_method(D_METHOD("get_logger"), &SentryEvent::get_logger);
	ClassDB::bind_method(D_METHOD("set_release", "release"), &SentryEvent::set_release);
	ClassDB::bind_method(D_METHOD("get_release"), &SentryEvent::get_release);
	ClassDB::bind_method(D_METHOD("set_dist", "dist"), &SentryEvent::set_dist);
	ClassDB::bind_method(D_METHOD("get_dist"), &SentryEvent::get_dist);
	ClassDB::bind_method(D_METHOD("set_environment", "environment"), &SentryEvent::set_environment);
	ClassDB::bind_method(D_METHOD("get_environment"), &SentryEvent::get_environment);
	ClassDB::bind_method(D_METHOD("set_tag", "key", "value"), &SentryEvent::set_tag);
	ClassDB::bind_method(D_METHOD("remove_tag", "key"), &SentryEvent::remove_tag);
	ClassDB::bind_method(D_METHOD("get_tag", "key"), &SentryEvent::get_tag);
	ClassDB::bind_method(D_METHOD("set_user", "user"), &SentryEvent::set_user);
	ClassDB::bind_method(D_METHOD("set_fingerprint", "fingerprint"), &SentryEvent::set_fingerprint);
	ClassDB::bind_method(D_METHOD("set_context", "key", "value"), &SentryEvent::set_context);
	ClassDB::bind_method(D_METHOD("is_crash"), &SentryEvent::is_crash);
	ClassDB::bind_method(D_METHOD("to_json"), &SentryEvent::to_json);

	// Runtime-generated fields have no serializable default.
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "message"), "set_message", "get_message");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "timestamp", PROPERTY_HINT_RESOURCE_TYPE, "SentryTimestamp"), "set_timestamp", "get_timestamp");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "platform", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_platform");
	ADD_PROPERTY(sentry::make_level_enum_property("level"), "set_level", "get_level");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "logger"), "set_logger", "get_logger");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "release"), "set_release", "get_release");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "dist"), "set_dist", "get_dist");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "environment"), "set_environment", "get_environment");

	ClassDB::bind_method(D_METHOD("get_exception_count"), &SentryEvent::get_exception_count);
	ClassDB::bind_method(D_METHOD("set_exception_value", "index", "value"), &SentryEvent::set_exception_value);
	ClassDB::bind_method(D_METHOD("get_exception_value", "index"), &SentryEvent::get_exception_value);
}

} // namespace sentry
