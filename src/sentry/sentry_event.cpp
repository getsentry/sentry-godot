#include "sentry_event.h"

#include "sentry_sdk.h" // Needed for VariantCaster<SentrySDK::Level>

#include <godot_cpp/classes/global_constants.hpp>

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
	ClassDB::bind_method(D_METHOD("is_crash"), &SentryEvent::is_crash);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "id"), "", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "message"), "set_message", "get_message");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "timestamp"), "set_timestamp", "get_timestamp");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "platform"), "", "get_platform");
	ADD_PROPERTY(sentry::make_level_enum_property("level"), "set_level", "get_level");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "logger"), "set_logger", "get_logger");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "release"), "set_release", "get_release");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "dist"), "set_dist", "get_dist");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "environment"), "set_environment", "get_environment");
}
